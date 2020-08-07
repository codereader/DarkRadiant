#include <zmq.hpp>
#undef min
#undef max
#include "GameConnection.h"
#include "MessageTCP.h"


zmq::context_t g_ZeroMqContext;
GameConnection g_gameConnection;

std::string SeqnoPreamble(int seq) {
    return fmt::format("seqno {}\n", seq);
}
std::string MessagePreamble(std::string type) {
    return fmt::format("message \"{}\"\n", type);
}
std::string ActionPreamble(std::string type) {
    return MessagePreamble("action") + fmt::format("action \"{}\"\n", type);
}
std::string QueryPreamble(std::string type) {
    return MessagePreamble("query") + fmt::format("query \"{}\"\n", type);
}

bool GameConnection::Connect() {
    if (_connection && _connection->IsAlive())
        return true;    //already connected

    if (_connection)
        _connection.reset();

    //connection using ZeroMQ socket
    std::unique_ptr<zmq::socket_t> zmqConnection(new zmq::socket_t(g_ZeroMqContext, ZMQ_STREAM));
    zmqConnection->connect(fmt::format("tcp://127.0.0.1:{}", 3879));

    _connection.reset(new MessageTcp());
    _connection->Init(std::move(zmqConnection));
    return _connection->IsAlive();
}

void GameConnection::Think() {
    _connection->Think();
    if (_seqnoInProgress) {
        //check if full response is here
        if (_connection->ReadMessage(_response)) {
            //mark request as "no longer in progress"
            //note: response can be used in outer function
            _seqnoInProgress = 0;
        }
    }
    else {
        //doing nothing now: send async command if present
        bool sentAsync = SendAsyncCommand();
    }
    _connection->Think();
}

void GameConnection::WaitAction() {
    if (_seqnoInProgress)
        Think();
}

bool GameConnection::SendAsyncCommand() {
    if (_cameraOutPending) {
        //TODO: send "setviewpos" action
        _cameraOutPending = false;
        return true;
    }
    return false;
}

void GameConnection::Finish() {
    //wait for current request in progress to finish
    WaitAction();
    //send pending async commands and wait for them to finish
    while (SendAsyncCommand())
        WaitAction();
}

std::string GameConnection::Execute(const std::string &request) {
    //make sure current request is finished (if any)
    WaitAction();
    assert(_seqnoInProgress == 0);

    //prepend seqno line and send message
    int seqno = g_gameConnection.NewSeqno();
    std::string fullMessage = SeqnoPreamble(seqno) + request;
    _connection->WriteMessage(fullMessage.data(), fullMessage.size());
    _seqnoInProgress = seqno;

    //wait until response is ready
    WaitAction();

    return std::string(_response.begin(), _response.end());
}

void GameConnection::ReloadMap(const cmd::ArgumentList& args) {
    if (!g_gameConnection.Connect())
        return;
    std::string text;
    text = ActionPreamble("conexec");
    text += "content:\n";
    text += "reloadMap\n";  //that's the console command we would execute
    g_gameConnection.Execute(text);
}
