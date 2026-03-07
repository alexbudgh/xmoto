/*=============================================================================
XMOTO

This file is part of XMOTO.

XMOTO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

XMOTO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XMOTO; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

#include "Sound.h"
#include "common/VFileIO.h"
#include "common/XMSession.h"
#include "helpers/Log.h"
#include "helpers/VExcept.h"
#include "include/xm_SDL.h"

/*===========================================================================
  Globals
  ===========================================================================*/
int Sound::m_nSampleRate;
int Sound::m_nSampleBits;
int Sound::m_nChannels;
bool Sound::m_activ;

std::vector<SoundSample *> Sound::m_Samples;
MIX_Mixer *Sound::m_pMixer = NULL;
MIX_Track *Sound::m_pMusicTrack = NULL;
MIX_Audio *Sound::m_pMenuMusic = NULL;
bool Sound::m_isInitialized = false;

void Sound::init(XMSession *i_session) {
  /* Get user configuration */

  int n;
  switch (n = i_session->audioSampleRate()) {
    case 11025:
    case 22050:
    case 44100:
      m_nSampleRate = n;
      break;
    default:
      LogWarning("invalid audio sample rate, falling back to 22050");
      m_nSampleRate = 22050;
      break;
  }

  switch (n = i_session->audioSampleBits()) {
    case 8:
    case 16:
      m_nSampleBits = n;
      break;
    default:
      LogWarning("invalid audio sample bits, falling back to 16");
      m_nSampleRate = 16;
      break;
  }

  m_nChannels = i_session->audioChannels();

  /* Init SDL stuff */
  if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    LogWarning("failed to initialize SDL audio (%s)", SDL_GetError());
    return;
  }

  if (!MIX_Init()) {
    LogWarning("failed to initialize SDL_mixer (%s)", SDL_GetError());
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return;
  }

  m_pMixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
  if (m_pMixer == NULL) {
    LogWarning("failed to create mixer device (%s)", SDL_GetError());
    MIX_Quit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return;
  }

  m_pMusicTrack = MIX_CreateTrack(m_pMixer);
  if (m_pMusicTrack == NULL) {
    LogWarning("failed to create music track (%s)", SDL_GetError());
    MIX_DestroyMixer(m_pMixer);
    m_pMixer = NULL;
    MIX_Quit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return;
  }

  m_pMenuMusic = NULL;
  m_activ = i_session->enableAudio();
  m_isInitialized = true;
}

void Sound::uninit(void) {
  /* Stop music */
  if (m_pMusicTrack != NULL) {
    MIX_StopTrack(m_pMusicTrack, 0);
    MIX_DestroyTrack(m_pMusicTrack);
    m_pMusicTrack = NULL;
  }

  if (m_pMenuMusic != NULL) {
    MIX_DestroyAudio(m_pMenuMusic);
    m_pMenuMusic = NULL;
  }

  /* Free loaded samples */
  for (unsigned int i = 0; i < m_Samples.size(); i++) {
    MIX_DestroyAudio(m_Samples[i]->pAudio);
    delete m_Samples[i];
  }
  m_Samples.clear();

  /* Destroy mixer */
  if (m_pMixer != NULL) {
    MIX_DestroyMixer(m_pMixer);
    m_pMixer = NULL;
  }

  MIX_Quit();

  /* Quit sound system if enabled */
  SDL_QuitSubSystem(SDL_INIT_AUDIO);

  m_isInitialized = false;
}

bool Sound::isInitialized() {
  return m_isInitialized;
}

void Sound::update(void) {}

Sint64 Sound::IOStream_size(void *userdata) {
  FileHandle *pf = (FileHandle *)userdata;
  int length = XMFS::getLength(pf);
  if (length < 0) return -1;
  return static_cast<Sint64>(length);
}

Sint64 Sound::IOStream_seek(void *userdata, Sint64 offset, SDL_IOWhence whence) {
  FileHandle *pf = (FileHandle *)userdata;
  switch (whence) {
    case SDL_IO_SEEK_SET:
      XMFS::setOffset(pf, offset);
      break;
    case SDL_IO_SEEK_END:
      XMFS::setOffset(pf, XMFS::getLength(pf));
      break;
    case SDL_IO_SEEK_CUR:
      XMFS::setOffset(pf, XMFS::getOffset(pf) + offset);
      break;
  }
  return XMFS::getOffset(pf);
}

size_t Sound::IOStream_read(void *userdata, void *ptr, size_t size, SDL_IOStatus *status) {
  FileHandle *pf = (FileHandle *)userdata;
  if (XMFS::isEnd(pf)) {
    if (status) *status = SDL_IO_STATUS_EOF;
    return 0;
  }

  int nRemaining = XMFS::getLength(pf) - XMFS::getOffset(pf);
  size_t nToRead = (size_t)nRemaining < size ? (size_t)nRemaining : size;

  if (!XMFS::readBuf(pf, (char *)ptr, nToRead)) {
    if (status) *status = SDL_IO_STATUS_ERROR;
    return 0;
  }

  if (status) *status = SDL_IO_STATUS_READY;
  return nToRead;
}

size_t Sound::IOStream_write(void *userdata, const void *ptr, size_t size, SDL_IOStatus *status) {
  if (status) *status = SDL_IO_STATUS_READY;
  return size;
}

bool Sound::IOStream_close(void *userdata) {
  return true;
}

SoundSample *Sound::loadSample(const std::string &File) {
  /* Allocate sample */
  SoundSample *pSample = new SoundSample;
  pSample->Name = File;

  /* Setup an IOStream interface */
  SDL_IOStreamInterface iface = {};
  iface.size = IOStream_size;
  iface.seek = IOStream_seek;
  iface.read = IOStream_read;
  iface.write = IOStream_write;
  iface.close = IOStream_close;

  FileHandle *pf = XMFS::openIFile(FDT_DATA, File);
  if (pf == NULL) {
    delete pSample;
    throw Exception("failed to open sample file " + File);
  }

  SDL_IOStream *io = SDL_OpenIO(&iface, (void *)pf);

  pSample->pAudio = MIX_LoadAudio_IO(m_pMixer, io, true, false);

  XMFS::closeFile(pf);
  SDL_CloseIO(io);

  if (pSample->pAudio == NULL) {
    delete pSample;
    throw Exception("failed to load sample " + File + ": " + SDL_GetError());
  }

  m_Samples.push_back(pSample);
  return pSample;
}

void Sound::playSample(SoundSample *pSample, float fVolume) {
  if (Sound::isActiv() == false)
    return;

  if (pSample == NULL || pSample->pAudio == NULL)
    return;

  if (fVolume >= 1.0f) {
    /* Fire-and-forget at full volume */
    MIX_PlayAudio(m_pMixer, pSample->pAudio);
  } else {
    /* Need a temporary track to set volume */
    MIX_Track *track = MIX_CreateTrack(m_pMixer);
    if (track != NULL) {
      MIX_SetTrackAudio(track, pSample->pAudio);
      MIX_SetTrackGain(track, fVolume);
      MIX_PlayTrack(track, 0);
      MIX_DestroyTrack(track);
    }
  }
}

SoundSample *Sound::findSample(const std::string &File) {
  for (unsigned int i = 0; i < m_Samples.size(); i++) {
    if (m_Samples[i]->Name == File)
      return m_Samples[i];
  }

  return loadSample(File);
}

void Sound::playSampleByName(const std::string &Name, float fVolume) {
  if (Sound::isActiv() == false)
    return;

  SoundSample *pSample = findSample(Name);
  if (pSample != NULL) {
    playSample(pSample, fVolume);
  }
}

/*==============================================================================
  Engine sound simulator
  ==============================================================================*/
void EngineSoundSimulator::update(int i_time) {
  if (Sound::isActiv() == false)
    return;

  if (i_time < m_lastBangTime)
    m_lastBangTime = i_time; /* manage back in the past */

  if (m_BangSamples.size() > 0) {
    if (m_fRPM > 100.0f) {
      /* Calculate the delay between the samples */
      int v_interval = (int)(60.0 * 120.0 / m_fRPM);

      if (i_time - m_lastBangTime > v_interval) {
        /* Stroke! Determine a random sample to use */
        float x = ((float)rand()) /
                  (float)RAND_MAX; /* linux likes insanely high RAND_MAX'es */
        int i = (int)(((float)m_BangSamples.size()) * x);
        if (i < 0)
          i = 0;
        if ((unsigned int)i >= m_BangSamples.size())
          i = m_BangSamples.size() - 1;
        /* Play it */
        MIX_PlayAudio(Sound::getMixer(), m_BangSamples[i]->pAudio);
        m_lastBangTime = i_time;
      }
    }
  }
}

void Sound::playMusic(std::string i_musicPath) {
  if (Sound::isActiv() == false)
    return;

  if (isPlayingMusic()) {
    stopMusic();
  }

  if (m_pMenuMusic != NULL) {
    MIX_DestroyAudio(m_pMenuMusic);
    m_pMenuMusic = NULL;
  }

  /* Load music */
  m_pMenuMusic = MIX_LoadAudio(m_pMixer, i_musicPath.c_str(), false);

  if (m_pMenuMusic == NULL) {
    throw Exception("No music played !");
  }

  MIX_SetTrackAudio(m_pMusicTrack, m_pMenuMusic);
  MIX_SetTrackLoops(m_pMusicTrack, -1);
  if (!MIX_PlayTrack(m_pMusicTrack, 0)) {
    throw Exception("No music played !");
  }
}

void Sound::togglePauseMusic() {
  if (m_pMenuMusic != NULL && MIX_TrackPlaying(m_pMusicTrack) &&
      MIX_TrackPaused(m_pMusicTrack) == false) {
    MIX_PauseTrack(m_pMusicTrack);
  } else if (m_pMenuMusic != NULL && MIX_TrackPlaying(m_pMusicTrack) && MIX_TrackPaused(m_pMusicTrack)) {
    MIX_ResumeTrack(m_pMusicTrack);
  }
}

void Sound::stopMusic() {
  if (m_pMenuMusic != NULL && MIX_TrackPlaying(m_pMusicTrack)) {
    MIX_StopTrack(m_pMusicTrack, 0);
  }
}

bool Sound::isPlayingMusic() {
  return m_pMusicTrack != NULL && MIX_TrackPlaying(m_pMusicTrack);
}

void Sound::setActiv(bool i_value) {
  m_activ = i_value;

  if (i_value == false && isPlayingMusic()) {
    stopMusic();
  }
}

bool Sound::isActiv() {
  return m_activ;
}
