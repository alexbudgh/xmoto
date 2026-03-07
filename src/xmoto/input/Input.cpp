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

/*
 *  Input handler
 */
#include "Input.h"
#include "InputLegacy.h"
#include "common/VFileIO.h"
#include "common/XMSession.h"
#include "db/xmDatabase.h"
#include "helpers/Log.h"
#include "helpers/Text.h"
#include "helpers/VExcept.h"
#include "xmoto/Game.h"
#include "xmoto/GameText.h"
#include "xmoto/SysMessage.h"
#include <sstream>
#include <utility>

Input::Input() {
  reset();
}

void Input::reset() {
  resetScriptKeyHooks();
}

bool Input::areJoysticksEnabled() const {
  return SDL_GamepadEventsEnabled();
}

void Input::enableJoysticks(bool i_value) {
  SDL_SetGamepadEventsEnabled(i_value);
}

/*===========================================================================
Init/uninit
===========================================================================*/
void Input::init(UserConfig *pConfig,
                 xmDatabase *pDb,
                 const std::string &i_id_profile,
                 bool i_enableJoysticks) {
  /* Initialize joysticks (if any) */
  SDL_InitSubSystem(SDL_INIT_GAMEPAD);

  enableJoysticks(i_enableJoysticks);

  loadJoystickMappings();

  /* Open all joysticks */
  recheckJoysticks();
  loadConfig(pConfig, pDb, i_id_profile);

  JoystickInput::instance()->init(m_joysticks.size());
}

void Input::uninit(void) {
  /* Close all joysticks */
  for (unsigned int i = 0; i < m_joysticks.size(); i++) {
    SDL_CloseGamepad(m_joysticks[i].handle);
  }

  /* No more joysticking */
  SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

/*===========================================================================
Read configuration
===========================================================================*/
void Input::loadConfig(UserConfig *pConfig,
                       xmDatabase *pDb,
                       const std::string &i_id_profile) {
  std::string v_key;

  /* Set defaults */
  setDefaultConfig();

  /* Get settings for mode */
  for (unsigned int player = 0; player < INPUT_NB_PLAYERS; player++) {
    std::string playerNumber = std::to_string(player + 1);

    for (IFullKey &f : m_controls[player].playerKeys) {
      try {
        f.key = XMKey(pDb->config_getString(
          i_id_profile, f.name + playerNumber, f.key.toString()));
      } catch (InvalidSystemKeyException &e) {
        /* keep default key */
      } catch (Exception &e) {
        // load default key (undefined) to not override the key while
        // undefined keys are not saved to avoid config brake in case
        // you forgot to plug your joystick
        f.key = XMKey();
      }
    }

    // script keys
    for (unsigned int k = 0; k < MAX_SCRIPT_KEY_HOOKS; k++) {
      IFullKey &f = m_controls[player].scriptActionKeys[k];

      v_key = pDb->config_getString(i_id_profile,
                                    "KeyActionScript" + playerNumber + "_" +
                                      std::to_string(k),
                                    "");
      if (v_key != "") { // don't override the default key if there is nothing
        // in the config
        try {
          f.key = XMKey(v_key);
        } catch (InvalidSystemKeyException &e) {
          /* keep default key */
        } catch (Exception &e) {
          f.key = XMKey();
        }
      }
    }
  }

  // global keys
  for (unsigned int i = 0; i < INPUT_NB_GLOBALKEYS; i++) {
    try {
      m_globalControls[i].key =
        XMKey(pDb->config_getString(i_id_profile,
                                    m_globalControls[i].name,
                                    m_globalControls[i].key.toString()));
    } catch (InvalidSystemKeyException &e) {
      /* keep default key */
    } catch (Exception &e) {
      m_globalControls[i].key = XMKey();
    }
  }
}

/*===========================================================================
Add script key hook
===========================================================================*/
void Input::addScriptKeyHook(Scene *pGame,
                             const std::string &keyName,
                             const std::string &FuncName) {
  if (m_nNumScriptKeyHooks < MAX_SCRIPT_KEY_HOOKS) {
    m_ScriptKeyHooks[m_nNumScriptKeyHooks].FuncName = FuncName;

    if (keyName.size() == 1) { /* old basic mode */
      m_ScriptKeyHooks[m_nNumScriptKeyHooks].nKey = XMKey(keyName, true);
    } else {
      m_ScriptKeyHooks[m_nNumScriptKeyHooks].nKey = XMKey(keyName);
    }
    m_ScriptKeyHooks[m_nNumScriptKeyHooks].pGame = pGame;
    m_nNumScriptKeyHooks++;
  }
}

int Input::getNumScriptKeyHooks() const {
  return m_nNumScriptKeyHooks;
}

InputScriptKeyHook Input::getScriptKeyHooks(int i) const {
  return m_ScriptKeyHooks[i];
}

XMKey Input::getScriptActionKeys(int i_player, int i_actionScript) const {
  return m_controls[i_player].scriptActionKeys[i_actionScript].key;
}

SDL_JoystickID Input::getJoyNum(const Joystick &joystick) const {
  for (unsigned int i = 0; i < m_joysticks.size(); i++) {
    if (m_joysticks[i].id == joystick.id) {
      return i;
    }
  }
  throw Exception("Invalid joystick name");
}

Joystick *Input::getJoyById(SDL_JoystickID id) {
  for (unsigned int i = 0; i < m_joysticks.size(); i++) {
    if (m_joysticks[i].id == id) {
      return &m_joysticks[i];
    }
  }
  throw Exception("Invalid joystick id");
}

InputEventType Input::joystickAxisSens(int16_t m_joyAxisValue) {
  return JoystickInput::instance()->axisInside(m_joyAxisValue,
                                               JOYSTICK_DEADZONE_BASE)
           ? INPUT_UP
           : INPUT_DOWN;
}

/*===========================================================================
Set totally default configuration - useful for when something goes wrong
===========================================================================*/
void Input::setDefaultConfig() {
  m_controls[0].playerKeys[INPUT_DRIVE] =
    IFullKey("KeyDrive", XMKey(SDLK_UP, SDL_KMOD_NONE), GAMETEXT_DRIVE);
  m_controls[0].playerKeys[INPUT_BRAKE] =
    IFullKey("KeyBrake", XMKey(SDLK_DOWN, SDL_KMOD_NONE), GAMETEXT_BRAKE);
  m_controls[0].playerKeys[INPUT_FLIPLEFT] =
    IFullKey("KeyFlipLeft", XMKey(SDLK_LEFT, SDL_KMOD_NONE), GAMETEXT_FLIPLEFT);
  m_controls[0].playerKeys[INPUT_FLIPRIGHT] =
    IFullKey("KeyFlipRight", XMKey(SDLK_RIGHT, SDL_KMOD_NONE), GAMETEXT_FLIPRIGHT);
  m_controls[0].playerKeys[INPUT_CHANGEDIR] =
    IFullKey("KeyChangeDir", XMKey(SDLK_SPACE, SDL_KMOD_NONE), GAMETEXT_CHANGEDIR);

  m_controls[1].playerKeys[INPUT_DRIVE] =
    IFullKey("KeyDrive", XMKey(SDLK_A, SDL_KMOD_NONE), GAMETEXT_DRIVE);
  m_controls[1].playerKeys[INPUT_BRAKE] =
    IFullKey("KeyBrake", XMKey(SDLK_Q, SDL_KMOD_NONE), GAMETEXT_BRAKE);
  m_controls[1].playerKeys[INPUT_FLIPLEFT] =
    IFullKey("KeyFlipLeft", XMKey(SDLK_Z, SDL_KMOD_NONE), GAMETEXT_FLIPLEFT);
  m_controls[1].playerKeys[INPUT_FLIPRIGHT] =
    IFullKey("KeyFlipRight", XMKey(SDLK_E, SDL_KMOD_NONE), GAMETEXT_FLIPRIGHT);
  m_controls[1].playerKeys[INPUT_CHANGEDIR] =
    IFullKey("KeyChangeDir", XMKey(SDLK_W, SDL_KMOD_NONE), GAMETEXT_CHANGEDIR);

  m_controls[2].playerKeys[INPUT_DRIVE] =
    IFullKey("KeyDrive", XMKey(SDLK_R, SDL_KMOD_NONE), GAMETEXT_DRIVE);
  m_controls[2].playerKeys[INPUT_BRAKE] =
    IFullKey("KeyBrake", XMKey(SDLK_F, SDL_KMOD_NONE), GAMETEXT_BRAKE);
  m_controls[2].playerKeys[INPUT_FLIPLEFT] =
    IFullKey("KeyFlipLeft", XMKey(SDLK_T, SDL_KMOD_NONE), GAMETEXT_FLIPLEFT);
  m_controls[2].playerKeys[INPUT_FLIPRIGHT] =
    IFullKey("KeyFlipRight", XMKey(SDLK_Y, SDL_KMOD_NONE), GAMETEXT_FLIPRIGHT);
  m_controls[2].playerKeys[INPUT_CHANGEDIR] =
    IFullKey("KeyChangeDir", XMKey(SDLK_V, SDL_KMOD_NONE), GAMETEXT_CHANGEDIR);

  m_controls[3].playerKeys[INPUT_DRIVE] =
    IFullKey("KeyDrive", XMKey(SDLK_U, SDL_KMOD_NONE), GAMETEXT_DRIVE);
  m_controls[3].playerKeys[INPUT_BRAKE] =
    IFullKey("KeyBrake", XMKey(SDLK_J, SDL_KMOD_NONE), GAMETEXT_BRAKE);
  m_controls[3].playerKeys[INPUT_FLIPLEFT] =
    IFullKey("KeyFlipLeft", XMKey(SDLK_I, SDL_KMOD_NONE), GAMETEXT_FLIPLEFT);
  m_controls[3].playerKeys[INPUT_FLIPRIGHT] =
    IFullKey("KeyFlipRight", XMKey(SDLK_O, SDL_KMOD_NONE), GAMETEXT_FLIPRIGHT);
  m_controls[3].playerKeys[INPUT_CHANGEDIR] =
    IFullKey("KeyChangeDir", XMKey(SDLK_K, SDL_KMOD_NONE), GAMETEXT_CHANGEDIR);

  m_globalControls[INPUT_SWITCHUGLYMODE] = IFullKey(
    "KeySwitchUglyMode", XMKey(SDLK_F9, SDL_KMOD_NONE), GAMETEXT_SWITCHUGLYMODE);
  m_globalControls[INPUT_SWITCHBLACKLIST] = IFullKey(
    "KeySwitchBlacklist", XMKey(SDLK_B, SDL_KMOD_LCTRL), GAMETEXT_SWITCHBLACKLIST);
  m_globalControls[INPUT_SWITCHFAVORITE] = IFullKey(
    "KeySwitchFavorite", XMKey(SDLK_F3, SDL_KMOD_NONE), GAMETEXT_SWITCHFAVORITE);
  m_globalControls[INPUT_RESTARTLEVEL] = IFullKey(
    "KeyRestartLevel", XMKey(SDLK_RETURN, SDL_KMOD_NONE), GAMETEXT_RESTARTLEVEL);
  m_globalControls[INPUT_SHOWCONSOLE] = IFullKey(
    "KeyShowConsole", XMKey(SDLK_GRAVE, SDL_KMOD_NONE), GAMETEXT_SHOWCONSOLE);
  m_globalControls[INPUT_CONSOLEHISTORYPLUS] =
    IFullKey("KeyConsoleHistoryPlus",
             XMKey(SDLK_PLUS, SDL_KMOD_LCTRL),
             GAMETEXT_CONSOLEHISTORYPLUS);
  m_globalControls[INPUT_CONSOLEHISTORYMINUS] =
    IFullKey("KeyConsoleHistoryMinus",
             XMKey(SDLK_MINUS, SDL_KMOD_LCTRL),
             GAMETEXT_CONSOLEHISTORYMINUS);
  m_globalControls[INPUT_RESTARTCHECKPOINT] =
    IFullKey("KeyRestartCheckpoint",
             XMKey(SDLK_BACKSPACE, SDL_KMOD_NONE),
             GAMETEXT_RESTARTCHECKPOINT);
  m_globalControls[INPUT_CHAT] =
    IFullKey("KeyChat", XMKey(SDLK_C, SDL_KMOD_LCTRL), GAMETEXT_CHATDIALOG);
  m_globalControls[INPUT_CHATPRIVATE] = IFullKey(
    "KeyChatPrivate", XMKey(SDLK_P, SDL_KMOD_LCTRL), GAMETEXT_CHATPRIVATEDIALOG);
  m_globalControls[INPUT_LEVELWATCHING] = IFullKey(
    "KeyLevelWatching", XMKey(SDLK_TAB, SDL_KMOD_NONE), GAMETEXT_LEVELWATCHING);
  m_globalControls[INPUT_SWITCHPLAYER] = IFullKey(
    "KeySwitchPlayer", XMKey(SDLK_F2, SDL_KMOD_NONE), GAMETEXT_SWITCHPLAYER);
  m_globalControls[INPUT_SWITCHTRACKINGSHOTMODE] =
    IFullKey("KeySwitchTrackingshotMode",
             XMKey(SDLK_F4, SDL_KMOD_NONE),
             GAMETEXT_SWITCHTRACKINGSHOTMODE);
  m_globalControls[INPUT_NEXTLEVEL] =
    IFullKey("KeyNextLevel", XMKey(SDLK_PAGEUP, SDL_KMOD_NONE), GAMETEXT_NEXTLEVEL);
  m_globalControls[INPUT_PREVIOUSLEVEL] =
    IFullKey("KeyPreviousLevel",
             XMKey(SDLK_PAGEDOWN, SDL_KMOD_NONE),
             GAMETEXT_PREVIOUSLEVEL);
  m_globalControls[INPUT_SWITCHRENDERGHOSTTRAIL] =
    IFullKey("KeySwitchRenderGhosttrail",
             XMKey(SDLK_G, SDL_KMOD_LCTRL),
             GAMETEXT_SWITCHREDERGHOSTTRAIL);
  m_globalControls[INPUT_SCREENSHOT] =
    IFullKey("KeyScreenshot", XMKey(SDLK_F12, SDL_KMOD_NONE), GAMETEXT_SCREENSHOT);
  m_globalControls[INPUT_LEVELINFO] =
    IFullKey("KeyLevelInfo", XMKey(), GAMETEXT_LEVELINFO);
  m_globalControls[INPUT_SWITCHWWWACCESS] = IFullKey(
    "KeySwitchWWWAccess", XMKey(SDLK_F8, SDL_KMOD_NONE), GAMETEXT_SWITCHWWWACCESS);
  m_globalControls[INPUT_SWITCHFPS] =
    IFullKey("KeySwitchFPS", XMKey(SDLK_F7, SDL_KMOD_NONE), GAMETEXT_SWITCHFPS);
  m_globalControls[INPUT_SWITCHGFXQUALITYMODE] =
    IFullKey("KeySwitchGFXQualityMode",
             XMKey(SDLK_F10, SDL_KMOD_NONE),
             GAMETEXT_SWITCHGFXQUALITYMODE);
  m_globalControls[INPUT_SWITCHGFXMODE] = IFullKey(
    "KeySwitchGFXMode", XMKey(SDLK_F11, SDL_KMOD_NONE), GAMETEXT_SWITCHGFXMODE);
  m_globalControls[INPUT_SWITCHNETMODE] = IFullKey(
    "KeySwitchNetMode", XMKey(SDLK_N, SDL_KMOD_LCTRL), GAMETEXT_SWITCHNETMODE);
  m_globalControls[INPUT_SWITCHHIGHSCOREINFORMATION] =
    IFullKey("KeySwitchHighscoreInformation",
             XMKey(SDLK_W, SDL_KMOD_LCTRL),
             GAMETEXT_SWITCHHIGHSCOREINFORMATION);
  m_globalControls[INPUT_NETWORKADMINCONSOLE] =
    IFullKey("KeyNetworkAdminConsole",
             XMKey(SDLK_S, (SDL_Keymod)(SDL_KMOD_LCTRL | SDL_KMOD_LALT)),
             GAMETEXT_NETWORKADMINCONSOLE);
  m_globalControls[INPUT_SWITCHSAFEMODE] =
    IFullKey("KeySafeMode", XMKey(SDLK_F6, SDL_KMOD_NONE), GAMETEXT_SWITCHSAFEMODE);
  m_globalControls[INPUT_TOGGLESERVERCONN] =
    IFullKey("KeyToggleServerConn", XMKey(), GAMETEXT_CLIENTCONNECTDISCONNECT);

  // uncustomizable keys
  m_globalControls[INPUT_HELP] =
    IFullKey("KeyHelp", XMKey(SDLK_F1, SDL_KMOD_NONE), GAMETEXT_HELP, false);
  m_globalControls[INPUT_RELOADFILESTODB] = IFullKey("KeyReloadFilesToDb",
                                                     XMKey(SDLK_F5, SDL_KMOD_NONE),
                                                     GAMETEXT_RELOADFILESTODB,
                                                     false);
  m_globalControls[INPUT_PLAYINGPAUSE] =
    IFullKey("KeyPlayingPause",
             XMKey(SDLK_ESCAPE, SDL_KMOD_NONE),
             GAMETEXT_PLAYINGPAUSE,
             false); // don't set it to true while ESCAPE is not setable via the
                     // option as a key
  m_globalControls[INPUT_KILLPROCESS] = IFullKey(
    "KeyKillProcess", XMKey(SDLK_K, SDL_KMOD_LCTRL), GAMETEXT_KILLPROCESS, false);
  m_globalControls[INPUT_REPLAYINGREWIND] =
    IFullKey("KeyReplayingRewind",
             XMKey(SDLK_LEFT, SDL_KMOD_NONE),
             GAMETEXT_REPLAYINGREWIND,
             false);
  m_globalControls[INPUT_REPLAYINGFORWARD] =
    IFullKey("KeyReplayingForward",
             XMKey(SDLK_RIGHT, SDL_KMOD_NONE),
             GAMETEXT_REPLAYINGFORWARD,
             false);
  m_globalControls[INPUT_REPLAYINGPAUSE] =
    IFullKey("KeyReplayingPause",
             XMKey(SDLK_SPACE, SDL_KMOD_NONE),
             GAMETEXT_REPLAYINGPAUSE,
             false);
  m_globalControls[INPUT_REPLAYINGSTOP] =
    IFullKey("KeyReplayingStop",
             XMKey(SDLK_ESCAPE, SDL_KMOD_NONE),
             GAMETEXT_REPLAYINGSTOP,
             false);
  m_globalControls[INPUT_REPLAYINGFASTER] = IFullKey("KeyReplayingFaster",
                                                     XMKey(SDLK_UP, SDL_KMOD_NONE),
                                                     GAMETEXT_REPLAYINGFASTER,
                                                     false);
  m_globalControls[INPUT_REPLAYINGABITFASTER] =
    IFullKey("KeyReplayingABitFaster",
             XMKey(SDLK_UP, SDL_KMOD_LCTRL),
             GAMETEXT_REPLAYINGABITFASTER,
             false);
  m_globalControls[INPUT_REPLAYINGSLOWER] =
    IFullKey("KeyReplayingSlower",
             XMKey(SDLK_DOWN, SDL_KMOD_NONE),
             GAMETEXT_REPLAYINGSLOWER,
             false);
  m_globalControls[INPUT_REPLAYINGABITSLOWER] =
    IFullKey("KeyReplayingABitSlower",
             XMKey(SDLK_DOWN, SDL_KMOD_LCTRL),
             GAMETEXT_REPLAYINGABITSLOWER,
             false);

  for (int player = 0; player < INPUT_NB_PLAYERS; ++player) {
    for (auto &f : Input::instance()->m_controls[player].scriptActionKeys) {
      f.key = XMKey();
    }
  }
}

/*===========================================================================
Get key by action...
===========================================================================*/

std::string Input::getKeyByAction(const std::string &Action, bool i_tech) {
  for (unsigned int i = 0; i < INPUT_NB_PLAYERS; i++) {
    std::string n;

    if (i != 0) // nothing for player 0
      n = " " + std::to_string(i + 1);

    INPUT_PLAYERKEYS pkey;

    if (Action == "Drive" + n)
      pkey = INPUT_DRIVE;
    else if (Action == "Brake" + n)
      pkey = INPUT_BRAKE;
    else if (Action == "PullBack" + n)
      pkey = INPUT_FLIPLEFT;
    else if (Action == "PushForward" + n)
      pkey = INPUT_FLIPRIGHT;
    else if (Action == "ChangeDir" + n)
      pkey = INPUT_CHANGEDIR;
    else
      continue;

    auto &key = m_controls[i].playerKeys[pkey].key;
    return i_tech ? key.toString() : key.toFancyString();
  }
  return "?";
}

void Input::saveConfig(UserConfig *pConfig,
                       xmDatabase *pDb,
                       const std::string &i_id_profile) {
  pDb->config_setValue_begin();

  for (unsigned int i = 0; i < INPUT_NB_PLAYERS; i++) {
    std::ostringstream v_n;
    v_n << (i + 1);

    // player keys
    for (unsigned int j = 0; j < INPUT_NB_PLAYERKEYS; j++) {
      auto &playerKey = m_controls[i].playerKeys[j];
      auto &key = playerKey.key;

      if (key.isDefined() || isANotGameSetKey(&key)) {
        pDb->config_setString(
          i_id_profile, playerKey.name + v_n.str(), key.toString());
      }
    }

    // script keys
    for (unsigned int k = 0; k < MAX_SCRIPT_KEY_HOOKS; k++) {
      auto &key = m_controls[i].scriptActionKeys[k].key;

      if (key.isDefined() || isANotGameSetKey(&key)) {
        std::ostringstream v_k;
        v_k << (k);

        pDb->config_setString(i_id_profile,
                              "KeyActionScript" + v_n.str() + "_" + v_k.str(),
                              key.toString());
      }
    }
  }

  // global keys
  for (unsigned int i = 0; i < INPUT_NB_GLOBALKEYS; i++) {
    pDb->config_setString(i_id_profile,
                          m_globalControls[i].name,
                          m_globalControls[i].key.toString());
  }

  pDb->config_setValue_end();
}

void Input::setSCRIPTACTION(int i_player, int i_action, XMKey i_value) {
  m_controls[i_player].scriptActionKeys[i_action].key = i_value;
}

XMKey Input::getSCRIPTACTION(int i_player, int i_action) const {
  return m_controls[i_player].scriptActionKeys[i_action].key;
}

void Input::setGlobalKey(unsigned int INPUT_key, XMKey i_value) {
  m_globalControls[INPUT_key].key = i_value;
}

const XMKey *Input::getGlobalKey(unsigned int INPUT_key) const {
  return &(m_globalControls[INPUT_key].key);
}

std::string Input::getGlobalKeyHelp(unsigned int INPUT_key) const {
  return m_globalControls[INPUT_key].help;
}

bool Input::getGlobalKeyCustomizable(unsigned int INPUT_key) const {
  return m_globalControls[INPUT_key].customizable;
}

void Input::setPlayerKey(unsigned int INPUT_key, int i_player, XMKey i_value) {
  m_controls[i_player].playerKeys[INPUT_key].key = i_value;
}

const XMKey *Input::getPlayerKey(unsigned int INPUT_key, int i_player) const {
  return &(m_controls[i_player].playerKeys[INPUT_key].key);
}

std::string Input::getPlayerKeyHelp(unsigned int INPUT_key,
                                    int i_player) const {
  return m_controls[i_player].playerKeys[INPUT_key].help;
}

bool Input::isANotGameSetKey(XMKey *i_xmkey) const {
  for (unsigned int i = 0; i < INPUT_NB_PLAYERS; i++) {
    for (unsigned int j = 0; j < INPUT_NB_PLAYERKEYS; j++) {
      if ((*getPlayerKey(j, i)) == *i_xmkey) {
        return false;
      }
    }

    for (unsigned int k = 0; k < MAX_SCRIPT_KEY_HOOKS; k++) {
      if (m_controls[i].scriptActionKeys[k].key == *i_xmkey) {
        return false;
      }
    }
  }

  return true;
}

void Input::recheckJoysticks() {
  std::string joyName, joyId;
  int n;
  SDL_Gamepad *joystick;

  m_joysticks.clear();

  int count = 0;
  SDL_JoystickID *gamepadIds = SDL_GetGamepads(&count);

  if (!gamepadIds) {
    LogWarning("Failed to enumerate gamepads: %s", SDL_GetError());
    return;
  }

  for (int i = 0; i < count; i++) {
    SDL_JoystickID gpId = gamepadIds[i];

    if (!(joystick = SDL_OpenGamepad(gpId))) {
      joyName = SDL_GetGamepadNameForID(gpId);
      LogWarning("Failed to open joystick [%s], abort to open other joysticks",
                 joyName.c_str());
      break;
    }

    std::ostringstream id;
    n = 0;
    joyName = SDL_GetGamepadName(joystick);
    SDL_JoystickID joystickId =
      SDL_GetJoystickID(SDL_GetGamepadJoystick(joystick));

    // check if there is an other joystick with the same name
    for (unsigned int j = 0; j < m_joysticks.size(); j++) {
      if (m_joysticks[j].name == joyName) {
        n++;
      }
    }

    if (n > 0) {
      id << " " << (n + 1); // +1 to get an id name starting at 1
    }
    joyId = joyName + id.str();
    m_joysticks.push_back(Joystick{ joystick, joyName, joystickId });
  }

  LogInfo("%d compatible controllers found%s",
          count,
          count > 0 ? ":" : "");
  for (int i = 0; i < (int)m_joysticks.size(); i++) {
    uint16_t vendorId = SDL_GetJoystickVendorForID(gamepadIds[i]);
    uint16_t productId = SDL_GetJoystickProductForID(gamepadIds[i]);
    Logger::LogLevelMsg(
      LOG_INFO, "  %d: %s (0x%04x:0x%04x)", i + 1, m_joysticks[i].name.c_str(), vendorId, productId);
  }

  if (XMSession::instance()->debug()) {
    LogDebug("Controller mappings:");
    for (int i = 0; i < (int)m_joysticks.size(); i++) {
      char *mapping = SDL_GetGamepadMapping(m_joysticks[i].handle);

      if (mapping) {
        LogDebug("  %d: %s", i + 1, mapping);
        SDL_free(mapping);
      } else {
        LogDebug("  %d: %s (%s)", i + 1, "<not available>", SDL_GetError());
      }
    }
  }

  SDL_free(gamepadIds);
}

void Input::loadJoystickMappings() {
  const char *mappingFile = "gamecontrollerdb.txt";
  FileHandle *file = XMFS::openIFile(FDT_DATA, mappingFile);
  if (!file) {
    const char *error = "Failed to read joystick mapping file";
    SysMessage::instance()->displayError(error);
    LogWarning(error);
    return;
  }

  std::string data = XMFS::readFileToEnd(file);
  SDL_IOStream *io = SDL_IOFromConstMem(data.c_str(), data.size());

  if (SDL_AddGamepadMappingsFromIO(io, true) < 0) {
    LogWarning("Failed to set up joystick mappings: %s", SDL_GetError());
  } else {
    LogInfo("Joystick mappings loaded");
  }
  XMFS::closeFile(file);
}

InputEventType Input::eventState(uint32_t type) {
  switch (type) {
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN: /* fall through */
    case SDL_EVENT_KEY_DOWN: /* fall through */
    case SDL_EVENT_MOUSE_BUTTON_DOWN: /* fall through */
      return INPUT_DOWN;

    case SDL_EVENT_GAMEPAD_BUTTON_UP: /* fall through */
    case SDL_EVENT_KEY_UP: /* fall through */
    case SDL_EVENT_MOUSE_BUTTON_UP: /* fall through */
      return INPUT_UP;

    default:
      return INPUT_INVALID;
  }
}

IFullKey::IFullKey(const std::string &i_name,
                   const XMKey &i_defaultKey,
                   const std::string i_help,
                   bool i_customizable) {
  name = i_name;
  key = i_defaultKey;
  defaultKey = i_defaultKey;
  help = i_help;
  customizable = i_customizable;
}

IFullKey::IFullKey() {}
