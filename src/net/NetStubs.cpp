/*=============================================================================
Stub implementations of networking classes when ENABLE_NETWORKING is off.
All functions are no-ops or return safe defaults.
=============================================================================*/

#if !ENABLE_NETWORKING

#include "NetClient.h"
#include "NetServer.h"
#include "NetActions.h"

/* NetAction base */
NetAction::NetAction(bool) : m_source(-1), m_subsource(0) {}
NetAction::~NetAction() {}
void NetAction::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
void NetAction::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *, const void *, int) {}
void NetAction::setSource(int i_src, int i_subsrc) { m_source = i_src; m_subsource = i_subsrc; }
int NetAction::getSource() const { return m_source; }
int NetAction::getSubSource() const { return m_subsource; }
void NetAction::logStats() {}
void NetAction::getNetAction(NetActionU *, void *, unsigned int) {}
std::string NetAction::getLine(void *, unsigned int, unsigned int *) { return ""; }
std::string NetAction::getLineCheckUTF8(void *, unsigned int, unsigned int *) { return ""; }
unsigned int NetAction::m_biggestTCPPacketSent = 0;
unsigned int NetAction::m_biggestUDPPacketSent = 0;
unsigned int NetAction::m_nbTCPPacketsSent = 0;
unsigned int NetAction::m_nbUDPPacketsSent = 0;
unsigned int NetAction::m_TCPPacketsSizeSent = 0;
unsigned int NetAction::m_UDPPacketsSizeSent = 0;
char NetAction::m_buffer[NETACTION_MAX_PACKET_SIZE];

/* Macro to stub an NA_ class with default constructor */
#define STUB_NA_DEFAULT(cls, natype, forceTcp) \
  cls::cls() : NetAction(forceTcp) {} \
  cls::~cls() {} \
  void cls::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {} \
  std::string cls::ActionKey = #cls; \
  NetActionType cls::NAType = natype;

/* Macro to stub an NA_ class with a string constructor */
#define STUB_NA_STRING(cls, natype, forceTcp) \
  cls::cls(const std::string &) : NetAction(forceTcp) {} \
  cls::~cls() {} \
  void cls::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {} \
  std::string cls::ActionKey = #cls; \
  NetActionType cls::NAType = natype;

/* NA_udpBind */
NA_udpBind::NA_udpBind(const std::string &) : NetAction(true) {}
NA_udpBind::NA_udpBind(void *, unsigned int) : NetAction(true) {}
NA_udpBind::~NA_udpBind() {}
void NA_udpBind::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
std::string NA_udpBind::key() const { return ""; }
std::string NA_udpBind::ActionKey = "udpBind";
NetActionType NA_udpBind::NAType = TNA_udpBind;

/* NA_udpBindQuery */
STUB_NA_DEFAULT(NA_udpBindQuery, TNA_udpBindQuery, true)
NA_udpBindQuery::NA_udpBindQuery(void *, unsigned int) : NetAction(true) {}

/* NA_udpBindValidation */
STUB_NA_DEFAULT(NA_udpBindValidation, TNA_udpBindValidation, true)
NA_udpBindValidation::NA_udpBindValidation(void *, unsigned int) : NetAction(true) {}

/* NA_clientInfos */
NA_clientInfos::NA_clientInfos(int, const std::string &) : NetAction(true) {}
NA_clientInfos::NA_clientInfos(void *, unsigned int) : NetAction(true) {}
NA_clientInfos::~NA_clientInfos() {}
void NA_clientInfos::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
int NA_clientInfos::protocolVersion() const { return 0; }
std::string NA_clientInfos::udpBindKey() const { return ""; }
std::string NA_clientInfos::xmversion() const { return ""; }
std::string NA_clientInfos::ActionKey = "clientInfos";
NetActionType NA_clientInfos::NAType = TNA_clientInfos;

/* NA_chatMessagePP */
NA_chatMessagePP::NA_chatMessagePP(const std::string &, const std::string &, const std::vector<int> &) : NetAction(true) {}
NA_chatMessagePP::NA_chatMessagePP(void *, unsigned int) : NetAction(true) {}
NA_chatMessagePP::~NA_chatMessagePP() {}
void NA_chatMessagePP::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
std::string NA_chatMessagePP::getMessage() { return ""; }
const std::vector<int> &NA_chatMessagePP::privatePeople() const { static std::vector<int> e; return e; }
std::string NA_chatMessagePP::ActionKey = "chatMessagePP";
NetActionType NA_chatMessagePP::NAType = TNA_chatMessagePP;

/* NA_chatMessage */
NA_chatMessage::NA_chatMessage(const std::string &, const std::string &) : NetAction(true) {}
NA_chatMessage::NA_chatMessage(void *, unsigned int) : NetAction(true) {}
NA_chatMessage::~NA_chatMessage() {}
void NA_chatMessage::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
std::string NA_chatMessage::getMessage() { return ""; }
std::string NA_chatMessage::ActionKey = "chatMessage";
NetActionType NA_chatMessage::NAType = TNA_chatMessage;

/* NA_serverError */
NA_serverError::NA_serverError(const std::string &) : NetAction(true) {}
NA_serverError::NA_serverError(void *, unsigned int) : NetAction(true) {}
NA_serverError::~NA_serverError() {}
void NA_serverError::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
std::string NA_serverError::getMessage() { return ""; }
std::string NA_serverError::ActionKey = "serverError";
NetActionType NA_serverError::NAType = TNA_serverError;

/* NA_frame */
NA_frame::NA_frame(SerializedBikeState *) : NetAction(false) {}
NA_frame::NA_frame(void *, unsigned int) : NetAction(false) {}
NA_frame::~NA_frame() {}
void NA_frame::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
SerializedBikeState *NA_frame::getState() { return &m_state; }
std::string NA_frame::ActionKey = "frame";
NetActionType NA_frame::NAType = TNA_frame;

/* NA_changeName */
NA_changeName::NA_changeName(const std::string &) : NetAction(true) {}
NA_changeName::NA_changeName(void *, unsigned int) : NetAction(true) {}
NA_changeName::~NA_changeName() {}
void NA_changeName::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
std::string NA_changeName::getName() { return ""; }
std::string NA_changeName::ActionKey = "changeName";
NetActionType NA_changeName::NAType = TNA_changeName;

/* NA_clientsNumber */
NA_clientsNumber::NA_clientsNumber(int, int, int, int) : NetAction(true) {}
NA_clientsNumber::NA_clientsNumber(void *, unsigned int) : NetAction(true) {}
NA_clientsNumber::~NA_clientsNumber() {}
void NA_clientsNumber::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
int NA_clientsNumber::getNumberTCP() const { return 0; }
int NA_clientsNumber::getNumberUDP() const { return 0; }
int NA_clientsNumber::getNumberGhosts() const { return 0; }
int NA_clientsNumber::getNumberSlaves() const { return 0; }
std::string NA_clientsNumber::ActionKey = "clientsNumber";
NetActionType NA_clientsNumber::NAType = TNA_clientsNumber;

/* NA_clientsNumberQuery */
STUB_NA_DEFAULT(NA_clientsNumberQuery, TNA_clientsNumberQuery, true)
NA_clientsNumberQuery::NA_clientsNumberQuery(void *, unsigned int) : NetAction(true) {}

/* NA_playingLevel */
NA_playingLevel::NA_playingLevel(const std::string &) : NetAction(true) {}
NA_playingLevel::NA_playingLevel(void *, unsigned int) : NetAction(true) {}
NA_playingLevel::~NA_playingLevel() {}
void NA_playingLevel::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
std::string NA_playingLevel::getLevelId() { return ""; }
std::string NA_playingLevel::ActionKey = "playingLevel";
NetActionType NA_playingLevel::NAType = TNA_playingLevel;

/* NA_changeClients */
NA_changeClients::NA_changeClients() : NetAction(true) {}
NA_changeClients::NA_changeClients(void *, unsigned int) : NetAction(true) {}
NA_changeClients::~NA_changeClients() {}
void NA_changeClients::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
void NA_changeClients::add(NetInfosClient *) {}
void NA_changeClients::remove(NetInfosClient *) {}
const std::vector<NetInfosClient> &NA_changeClients::getAddedInfosClients() const { static std::vector<NetInfosClient> e; return e; }
const std::vector<NetInfosClient> &NA_changeClients::getRemovedInfosClients() const { static std::vector<NetInfosClient> e; return e; }
std::string NA_changeClients::ActionKey = "changeClients";
NetActionType NA_changeClients::NAType = TNA_changeClients;

/* NA_playerControl */
NA_playerControl::NA_playerControl(PlayerControl, bool) : NetAction(false), m_control(PC_CHANGEDIR), m_value(0) {}
NA_playerControl::NA_playerControl(PlayerControl, float) : NetAction(false), m_control(PC_CHANGEDIR), m_value(0) {}
NA_playerControl::NA_playerControl(void *, unsigned int) : NetAction(false), m_control(PC_CHANGEDIR), m_value(0) {}
NA_playerControl::~NA_playerControl() {}
void NA_playerControl::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
PlayerControl NA_playerControl::getType() { return m_control; }
float NA_playerControl::getFloatValue() { return m_value; }
bool NA_playerControl::getBoolValue() { return m_value != 0.0f; }
std::string NA_playerControl::ActionKey = "playerControl";
NetActionType NA_playerControl::NAType = TNA_playerControl;

/* NA_clientMode */
NA_clientMode::NA_clientMode(NetClientMode) : NetAction(true), m_mode(NETCLIENT_GHOST_MODE) {}
NA_clientMode::NA_clientMode(void *, unsigned int) : NetAction(true), m_mode(NETCLIENT_GHOST_MODE) {}
NA_clientMode::~NA_clientMode() {}
void NA_clientMode::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
NetClientMode NA_clientMode::mode() const { return m_mode; }
std::string NA_clientMode::ActionKey = "clientMode";
NetActionType NA_clientMode::NAType = TNA_clientMode;

/* NA_prepareToPlay */
NA_prepareToPlay::NA_prepareToPlay(const std::string &, std::vector<int> &) : NetAction(true) {}
NA_prepareToPlay::NA_prepareToPlay(void *, unsigned int) : NetAction(true) {}
NA_prepareToPlay::~NA_prepareToPlay() {}
void NA_prepareToPlay::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
std::string NA_prepareToPlay::idLevel() const { return ""; }
const std::vector<int> &NA_prepareToPlay::players() { static std::vector<int> e; return e; }
std::string NA_prepareToPlay::ActionKey = "prepareToPlay";
NetActionType NA_prepareToPlay::NAType = TNA_prepareToPlay;

/* NA_killAlert */
NA_killAlert::NA_killAlert(int) : NetAction(true), m_time(0) {}
NA_killAlert::NA_killAlert(void *, unsigned int) : NetAction(true), m_time(0) {}
NA_killAlert::~NA_killAlert() {}
void NA_killAlert::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
int NA_killAlert::time() const { return m_time; }
std::string NA_killAlert::ActionKey = "killAlert";
NetActionType NA_killAlert::NAType = TNA_killAlert;

/* NA_prepareToGo */
NA_prepareToGo::NA_prepareToGo(int) : NetAction(true), m_time(0) {}
NA_prepareToGo::NA_prepareToGo(void *, unsigned int) : NetAction(true), m_time(0) {}
NA_prepareToGo::~NA_prepareToGo() {}
void NA_prepareToGo::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
int NA_prepareToGo::time() const { return m_time; }
std::string NA_prepareToGo::ActionKey = "prepareToGo";
NetActionType NA_prepareToGo::NAType = TNA_prepareToGo;

/* NA_gameEvents */
NA_gameEvents::NA_gameEvents(DBuffer *) : NetAction(true), m_bufferLength(0) {}
NA_gameEvents::NA_gameEvents(void *, unsigned int) : NetAction(true), m_bufferLength(0) {}
NA_gameEvents::~NA_gameEvents() {}
void NA_gameEvents::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
char *NA_gameEvents::buffer() { return m_buffer; }
int NA_gameEvents::bufferSize() { return m_bufferLength; }
std::string NA_gameEvents::ActionKey = "gameEvents";
NetActionType NA_gameEvents::NAType = TNA_gameEvents;

/* NA_srvCmd */
NA_srvCmd::NA_srvCmd(const std::string &) : NetAction(true) {}
NA_srvCmd::NA_srvCmd(void *, unsigned int) : NetAction(true) {}
NA_srvCmd::~NA_srvCmd() {}
void NA_srvCmd::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
std::string NA_srvCmd::getCommand() { return ""; }
std::string NA_srvCmd::ActionKey = "srvCmd";
NetActionType NA_srvCmd::NAType = TNA_srvCmd;

/* NA_srvCmdAsw */
NA_srvCmdAsw::NA_srvCmdAsw(const std::string &) : NetAction(true) {}
NA_srvCmdAsw::NA_srvCmdAsw(void *, unsigned int) : NetAction(true) {}
NA_srvCmdAsw::~NA_srvCmdAsw() {}
void NA_srvCmdAsw::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
std::string NA_srvCmdAsw::getAnswer() { return ""; }
std::string NA_srvCmdAsw::ActionKey = "srvCmdAsw";
NetActionType NA_srvCmdAsw::NAType = TNA_srvCmdAsw;

/* NA_slaveClientsPoints */
NA_slaveClientsPoints::NA_slaveClientsPoints() : NetAction(true) {}
NA_slaveClientsPoints::NA_slaveClientsPoints(void *, unsigned int) : NetAction(true) {}
NA_slaveClientsPoints::~NA_slaveClientsPoints() {}
void NA_slaveClientsPoints::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
const std::vector<NetPointsClient> &NA_slaveClientsPoints::getPointsClients() const { static std::vector<NetPointsClient> e; return e; }
void NA_slaveClientsPoints::add(NetPointsClient *) {}
std::string NA_slaveClientsPoints::ActionKey = "slaveClientsPoints";
NetActionType NA_slaveClientsPoints::NAType = TNA_slaveClientsPoints;

/* NA_ping */
NA_ping::NA_ping(NA_ping *) : NetAction(false), m_id(0), m_isPong(false) {}
NA_ping::NA_ping(void *, unsigned int) : NetAction(false), m_id(0), m_isPong(false) {}
NA_ping::~NA_ping() {}
void NA_ping::send(TCPsocket *, UDPsocket *, UDPpacket *, IPaddress *) {}
int NA_ping::id() const { return m_id; }
bool NA_ping::isPong() const { return m_isPong; }
std::string NA_ping::ActionKey = "ping";
NetActionType NA_ping::NAType = TNA_ping;
int NA_ping::m_currentId = 0;

/* NetClient */
NetClient::NetClient() : m_isConnected(false), m_mode(NETCLIENT_GHOST_MODE), m_points(0), m_tcpReader(NULL), m_universe(NULL) {}
NetClient::~NetClient() {}
void NetClient::fastConnectDisconnect(const std::string &, int) {}
void NetClient::connect(const std::string &, int) {}
void NetClient::disconnect() {}
bool NetClient::isConnected() { return false; }
void NetClient::send(NetAction *, int, bool) {}
TCPsocket *NetClient::tcpSocket() { return NULL; }
UDPsocket *NetClient::udpSocket() { return NULL; }
UDPpacket *NetClient::sendPacket() { return NULL; }
std::string NetClient::udpBindKey() const { return ""; }
void NetClient::manageNetwork(int, xmDatabase *) {}
void NetClient::changeMode(NetClientMode) {}
NetClientMode NetClient::mode() const { return NETCLIENT_GHOST_MODE; }
int NetClient::points() const { return 0; }
void NetClient::startPlay(Universe *) {}
bool NetClient::isPlayInitialized() { return false; }
void NetClient::endPlay() {}
void NetClient::getOtherClientsNameList(std::vector<std::string> &, const std::string &) {}
void NetClient::addChatTransformations(std::vector<std::string> &, const std::string &) {}
std::string NetClient::getDisplayMessage(const std::string &i_msg, const std::string &) { return i_msg; }
std::vector<NetOtherClient *> &NetClient::otherClients() { static std::vector<NetOtherClient *> empty; return empty; }
void NetClient::fillPrivatePeople(const std::string &, const std::string &, std::vector<int> &, std::vector<std::string> &) {}
int NetClient::getOwnFrameFPS() const { return 0; }
VirtualNetLevelsList *NetClient::getOtherClientLevelsList(xmDatabase *) { return NULL; }
void NetClient::memoriesPP(const std::vector<int> &) {}
std::string NetClient::getMemoriedPPAsString(const std::string &) { return ""; }

/* NetOtherClient */
NetOtherClient::NetOtherClient(int i_id, const std::string &i_name) : m_id(i_id), m_name(i_name), m_netMode(NETCLIENT_GHOST_MODE), m_points(0) {
  for (int i = 0; i < NETACTION_MAX_SUBSRC; i++) m_ghosts[i] = NULL;
}
NetOtherClient::~NetOtherClient() {}
int NetOtherClient::id() const { return m_id; }
std::string NetOtherClient::name() const { return m_name; }
NetClientMode NetOtherClient::mode() const { return m_netMode; }
void NetOtherClient::setName(const std::string &i_name) { m_name = i_name; }
int NetOtherClient::points() const { return m_points; }
void NetOtherClient::setPoints(int p) { m_points = p; }
std::string NetOtherClient::lastPlayingLevelId() { return ""; }
void NetOtherClient::setPlayingLevelId(xmDatabase *, const std::string &) {}
std::string NetOtherClient::playingLevelName() { return ""; }
NetGhost *NetOtherClient::netGhost(unsigned int) { return NULL; }
void NetOtherClient::setNetGhost(unsigned int, NetGhost *) {}

/* NetServer */
NetServer::NetServer() : m_serverThread(NULL), m_isStarted(false) {}
NetServer::~NetServer() {}
bool NetServer::isStarted() { return false; }
void NetServer::start(bool, int, const std::string &) {}
void NetServer::stop() {}
bool NetServer::acceptConnections() { return false; }
void NetServer::wait() {}
void NetServer::setStandAloneOptions() {}

#endif
