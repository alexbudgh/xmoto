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

#ifndef __SOUND_H__
#define __SOUND_H__

#include "common/VCommon.h"
#include "common/VFileIO.h"
#include "include/xm_SDL_mixer.h"
#define DEFAULT_SAMPLE_VOLUME 1.0f

class XMSession;

/*===========================================================================
Sound sample
===========================================================================*/
struct SoundSample {
  int nBufSize;
  unsigned char *pcBuf;
  std::string Name;

  MIX_Audio *pAudio;
};

/*===========================================================================
Engine sound simulator (single cylinder, 4-stroke)
===========================================================================*/
class EngineSoundSimulator {
public:
  EngineSoundSimulator() {
    m_fRPM = 0.0f;
    m_lastBangTime = 0;
  }

  /* Methods */
  void update(int i_time);

  /* Data interface */
  void setRPM(float f) { m_fRPM = f; }
  float getRPM(void) { return m_fRPM; }
  void addBangSample(SoundSample *pSample) {
    if (pSample != NULL)
      m_BangSamples.push_back(pSample);
  }

private:
  /* Data */
  std::vector<SoundSample *> m_BangSamples;
  float m_fRPM;
  int m_lastBangTime;
};

/*===========================================================================
Sound system object
===========================================================================*/
class Sound {
public:
  /* Static functions */
  static void init(XMSession *i_session);
  static void uninit(void);
  static bool isInitialized();

  static void update(void);

  static SoundSample *loadSample(const std::string &File);
  static void playSample(SoundSample *pSample,
                         float fVolume = DEFAULT_SAMPLE_VOLUME);
  static SoundSample *findSample(const std::string &File);
  static void playSampleByName(const std::string &Name,
                               float fVolume = DEFAULT_SAMPLE_VOLUME);

  /* Data interface */
  static int getSampleRate(void) { return m_nSampleRate; }
  static int getSampleBits(void) { return m_nSampleBits; }
  static int getChannels(void) { return m_nChannels; }
  static int getNumSamples(void) { return m_Samples.size(); }

  static void playMusic(std::string i_musicPath);
  static void togglePauseMusic();
  static void stopMusic();
  static bool isPlayingMusic();

  static void setActiv(bool i_value);
  static bool isActiv();

  static bool m_activ;

  static MIX_Mixer *getMixer() { return m_pMixer; }

private:
  /* SDL_mixer callbacks (IOStream) */
  static Sint64 IOStream_size(void *userdata);
  static Sint64 IOStream_seek(void *userdata, Sint64 offset, SDL_IOWhence whence);
  static size_t IOStream_read(void *userdata, void *ptr, size_t size, SDL_IOStatus *status);
  static size_t IOStream_write(void *userdata, const void *ptr, size_t size, SDL_IOStatus *status);
  static bool IOStream_close(void *userdata);

  /* Data */
  static int m_nSampleRate;
  static int m_nSampleBits;
  static int m_nChannels;

  static std::vector<SoundSample *> m_Samples;

  static MIX_Mixer *m_pMixer;
  static MIX_Track *m_pMusicTrack;
  static MIX_Audio *m_pMenuMusic;
  static bool m_isInitialized;
};

#endif
