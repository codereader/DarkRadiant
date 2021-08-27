#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <stdexcept>

namespace gameconn
{

class MessageTcp;

// Thrown by some methods of AutomationEngine and GameConnection
// if connection was lost or was not established at all.
class DisconnectException : public std::runtime_error {
public:
    DisconnectException() : std::runtime_error("Game connection lost") {}
};

// Bitmask with all tag bits set.
static const int TAGMASK_ALL = -1;

// Putting this to wait-list of multistep procedure
// results in waking up on next think (suitable for polling).
static const int SEQNO_WAIT_POLL = -10000;

struct MultistepProcReturn {
    // Which step will be executed next time.
    int nextStep;
    // Resume procedure when all of these requests are finished.
    // If it is empty, then procedure is continued immediately.
    // If only SEQNO_WAIT_POLL remains, then procedure is resumed on next think.
    std::vector<int> seqnoWaitList;
};


/**
 * stgatilov: The engine for connection to TheDarkMod's automation via socket.
 */
class AutomationEngine
{
public:
    AutomationEngine();
    ~AutomationEngine();


    // Connect to TDM instance if not connected yet.
    // Returns false if failed to connect, true on success.
    bool connect();
    // Disconnect from TDM instance if connected.
    // If force = false, then it waits until all pending requests are finished.
    // If force = true, then all pending requests are dropped, no blocking for sure.
    void disconnect(bool force = false);

    // Returns false if connection is not yet established or has been closed for whatever reason.
    bool isAlive() const;
    // Returns true if connection was established, but was lost after that and disconnect was not called yet.
    bool hasLostConnection() const;


    // Check how socket is doing, accept responses, call callbacks.
    // This should be done regularly, e.g. in a timer.
    void think();


    // Send given request synchronously, i.e. wait until its completition.
    // Returns response content.
    // Throws DisconnectException if connection is missing or lost during execution.
    std::string executeRequestBlocking(int tag, const std::string& request);

    // Send request !a!synchronously, i.e. send it to socket and return immediately.
    // Returns seqno of the request: it can be put to wait-list of multistep procedure or used in queries.
    // When request is finished, optional callback will be executed (from "think" method).
    // Note that if connection is lost, NO special callback is called.
    // May throw DisconnectException if connection is missing (but never throws if isAlive() is true before call).
    int executeRequestAsync(int tag, const std::string& request, const std::function<void(int)>& callback = {});

    // Execute given multistep procedure, starting on the next think.
    // Returns ID of procedure for queries.
    // Multistep procedure is like a DFA of "steps", each step is executed till start to end.
    // When step is over, you can specify async requests to wait for, and which step to resume when they are finished.
    int executeMultistepProc(int tag, const std::function<MultistepProcReturn(int)>& function, int startStep = 0);


    // Return true iff any of the given requests or multistep procedures are not finished yet.
    bool areInProgress(const std::vector<int>& reqSeqnos, const std::vector<int>& procIds);
    // Wait for specified requests and multistep procedures to finish.
    // Throws DisconnectException if it cannot be done due to lost connection.
    void wait(const std::vector<int>& reqSeqnos, const std::vector<int>& procIds);

    // Return true iff any request or multistep procedure matching the given mask is not finished yet.
    // Note: tagMask is a bitmask, so pass (1<<tag) in order to wait for one tag only.
    bool areTagsInProgress(int tagMask = TAGMASK_ALL);
    // Wait for all active requests and multistep procedures matching the given mask to finish.
    // Throws DisconnectException if it cannot be done due to lost connection.
    void waitForTags(int tagMask = TAGMASK_ALL);


    // Returns response for just finished request with given seqno.
    // Can be called inside:
    //   * executeRequestAsync callback
    //   * multistep procedure
    std::string getResponse(int seqno) const;

private:
    // Connection to TDM game (i.e. the socket with custom message framing).
    // It can be "dead" in two ways:
    //   _connection is NULL --- no connection
    //   *_connection is dead --- just lost connection
    std::unique_ptr<MessageTcp> _connection;
    // Sequence number of the last sent request (incremented sequentally).
    int _seqno = 0;
    // Number of multistep procedures ever seen (incremented sequentally).
    int _procIds = 0;
    // Used for counting recursive depth of think method.
    int _thinkDepth = 0;

    struct Request {
        int _seqno = 0;
        int _tag = 0;
        bool _finished = false;
        std::string _request;
        std::string _response;

        std::function<void(int)> _callback;
    };
    std::vector<Request> _requests;

    struct MultistepProcedure {
        int _id = 0;
        int _tag = 0;
        std::vector<int> _waitForSeqnos;
        std::function<MultistepProcReturn(int)> _function;
        int _currentStep = -1;
    };
    std::vector<MultistepProcedure> _multistepProcs;

    int generateNewSequenceNumber();
    Request* sendRequest(int tag, const std::string& request);

    Request* findRequest(int seqno) const;
    MultistepProcedure* findMultistepProc(int id) const;

    bool isMultistepProcStillWaiting(const MultistepProcedure& proc, bool waitForPoll) const;
    void resumeMultistepProcedure(int id);
};

}
