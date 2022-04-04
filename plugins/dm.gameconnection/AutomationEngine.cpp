#include "AutomationEngine.h"

#include <cassert>

#include "MessageTcp.h"
#include "clsocket/ActiveSocket.h"

#include <fmt/format.h>


namespace gameconn
{

namespace
{
    const char* const DEFAULT_HOST = "localhost";
    const int DEFAULT_PORT = 3879;

    inline std::string seqnoPreamble(std::size_t seq) {
        return fmt::format("seqno {0}\n", seq);
    }

    struct ScopedDepthCounter {
        int& ref;
        ScopedDepthCounter(int& counter) : ref(counter) {
            ref++;
        }
        ~ScopedDepthCounter() {
            ref--;
        }
    };
}

AutomationEngine::AutomationEngine()
{}
AutomationEngine::~AutomationEngine()
{
    disconnect(true);
}

bool AutomationEngine::isAlive() const
{
    return _connection && _connection->isAlive();
}
bool AutomationEngine::hasLostConnection() const
{
    return _connection && !_connection->isAlive();
}

bool AutomationEngine::connect()
{
    if (isAlive())
        return true;    //already connected

    // Make connection using clsocket
    // TODO: make port configurable, as it is in TDM?
    std::unique_ptr<CActiveSocket> connection(new CActiveSocket());
    if (
        !connection->Initialize() ||
        !connection->SetNonblocking() ||
        !connection->Open(DEFAULT_HOST, DEFAULT_PORT))
    {
        return false;
    }

    _connection.reset(new MessageTcp());
    _connection->init(std::move(connection));
    if (!_connection->isAlive())
        return false;

    return true;
}

void AutomationEngine::disconnect(bool force)
{
    if (force) {
        //drop everything pending 
        _multistepProcs.clear();
        _requests.clear();
    }
    else {
        //try to finish all pending
        waitForTags();
    }

    if (_connection)
        _connection.reset();
}


int AutomationEngine::generateNewSequenceNumber()
{
    return ++_seqno;
}

AutomationEngine::Request* AutomationEngine::sendRequest(int tag, const std::string& request) {
    assert(tag < 31);
    if (!_connection)
        throw DisconnectException();

    Request req;
    req._seqno = generateNewSequenceNumber();
    req._request = request;
    req._tag = tag;
    req._finished = false;

    std::string fullMessage = seqnoPreamble(req._seqno) + req._request;
    _connection->writeMessage(fullMessage.data(), (int)fullMessage.size());
    _requests.push_back(req);

    return &_requests.back();
}

void AutomationEngine::think() {
    ScopedDepthCounter dc(_thinkDepth);

    if (_connection) {
        _connection->think();

        //check if full response is here
        std::vector<char> responseBytes;
        while (_connection->readMessage(responseBytes)) {
            //validate and remove preamble
            int responseSeqno, lineLen;
            int ret = sscanf(responseBytes.data(), "response %d\n%n", &responseSeqno, &lineLen);
            assert(ret == 1); ret;
            std::string response(responseBytes.begin() + lineLen, responseBytes.end());

            //find request, mark it as "no longer in progress"
            if (Request* req = findRequest(responseSeqno)) {
                req->_finished = true;
                req->_response = response;
            }
        }
    }

    //execute callbacks for finished requests
    for (int i = 0; i < _requests.size(); i++) {
        if (_requests[i]._finished && _requests[i]._callback) {
            _requests[i]._callback(_requests[i]._seqno);
            _requests[i]._callback = {};
        }
    }

    //don't do some stuff in nested think calls:
    //  1) don't delete finished requests (multistep proc might still need it)
    //  2) don't resume multistep procedures (to avoid recursion)
    if (_thinkDepth == 1) {
        //resume multistep procedures
        for (int i = 0; i < _multistepProcs.size(); i++) {
            MultistepProcedure& proc = _multistepProcs[i];
            if (!isMultistepProcStillWaiting(proc, false)) {
                //BEWARE: it may call blocking requests,
                //which in turn call think recursively!
                resumeMultistepProcedure(proc._id);
            }
        }

        //remove finished requests
        int k = 0;
        for (int i = 0; i < _requests.size(); i++) {
            if (!_requests[i]._finished)
                _requests[k++] = _requests[i];
        }
        _requests.resize(k);

        //remove finished multistep procedures
        k = 0;
        for (int i = 0; i < _multistepProcs.size(); i++) {
            if (_multistepProcs[i]._currentStep >= 0)
                _multistepProcs[k++] = _multistepProcs[i];
        }
        _multistepProcs.resize(k);
    }
}

bool AutomationEngine::isMultistepProcStillWaiting(const MultistepProcedure& proc, bool waitForPoll) const
{
    bool stillWaits = false;

    for (int j = 0; j < proc._waitForSeqnos.size() && !stillWaits; j++) {
        int waitSeqno = proc._waitForSeqnos[j];

        if (waitSeqno == SEQNO_WAIT_POLL) {
            if (waitForPoll)
                stillWaits = true;
        }
        else if (Request* req = findRequest(waitSeqno)) {
            if (!req->_finished)
                stillWaits = true;
        }
    }

    return stillWaits;
}

void AutomationEngine::resumeMultistepProcedure(int id)
{
    MultistepProcedure* proc;
    do {
        proc = findMultistepProc(id);
        assert(proc);

        if (proc->_currentStep < 0)
            break;  //finished

        auto ret = proc->_function(proc->_currentStep);

        proc->_currentStep = ret.nextStep;
        proc->_waitForSeqnos = ret.seqnoWaitList;
    }
    while (!isMultistepProcStillWaiting(*proc, true));
}

bool AutomationEngine::areInProgress(const std::vector<int>& reqSeqnos, const std::vector<int>& procIds)
{
    for (int seqno : reqSeqnos)
        if (Request* req = findRequest(seqno))
            if (!req->_finished)
                return true;
    for (int id : procIds)
        if (MultistepProcedure* proc = findMultistepProc(id))
            if (proc->_currentStep >= 0)
                return true;
    return false;
}

bool AutomationEngine::areTagsInProgress(int tagMask)
{
    for (int i = 0; i < _requests.size(); i++)
        if (tagMask & (1 << _requests[i]._tag))
            if (!_requests[i]._finished)
                return true;
    for (int i = 0; i < _multistepProcs.size(); i++)
        if (tagMask & (1 << _multistepProcs[i]._tag))
            if (_multistepProcs[i]._currentStep >= 0)
                return true;
    return false;
}

void AutomationEngine::wait(const std::vector<int>& reqSeqnos, const std::vector<int>& procIds)
{
    while (areInProgress(reqSeqnos, procIds)) {
        if (!isAlive())
            throw DisconnectException();
        think();
    }
}

void AutomationEngine::waitForTags(int tagMask)
{
    while (areTagsInProgress(tagMask)) {
        if (!isAlive())
            throw DisconnectException();
        think();
    }
}

std::string AutomationEngine::executeRequestBlocking(int tag, const std::string& request)
{
    Request* req = sendRequest(tag, request);
    int seqno = req->_seqno;

    std::string response;
    req->_callback = [this, seqno, &response](int num) {
        Request *req = findRequest(seqno);
        assert(num == seqno && req && req->_finished);
        response = req->_response;
    };

    wait({seqno}, {});

    return response;
}

int AutomationEngine::executeRequestAsync(int tag, const std::string& request, const std::function<void(int)>& callback)
{
    Request* req = sendRequest(tag, request);
    req->_callback = callback;
    return req->_seqno;
}

int AutomationEngine::executeMultistepProc(int tag, const std::function<MultistepProcReturn(int)>& function, int startStep)
{
    assert(tag < 31);
    MultistepProcedure proc;
    proc._id = ++_procIds;
    proc._tag = tag;
    proc._function = function;
    proc._currentStep = startStep;
    _multistepProcs.push_back(proc);
    return proc._id;
}

AutomationEngine::Request* AutomationEngine::findRequest(int seqno) const
{
    for (int i = 0; i < _requests.size(); i++) {
        if (_requests[i]._seqno == seqno)
            return const_cast<Request*>(&_requests[i]);
    }
    return nullptr;
}

AutomationEngine::MultistepProcedure* AutomationEngine::findMultistepProc(int id) const
{
    for (int i = 0; i < _multistepProcs.size(); i++) {
        if (_multistepProcs[i]._id == id)
            return const_cast<MultistepProcedure*>(&_multistepProcs[i]);
    }
    return nullptr;
}

std::string AutomationEngine::getResponse(int seqno) const
{
    const Request* req = findRequest(seqno);

    //TODO: perhaps we should simply keep finished requests in memory forever?...
    assert(req);
    if (!req)
        return "";

    assert(req->_finished);
    return req->_response;
}

}