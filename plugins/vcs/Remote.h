#pragma once

#include <git2.h>
#include "Repository.h"
#include "CredentialManager.h"
#include "GitException.h"
#include <wx/uri.h>
#include <fmt/format.h>

namespace vcs
{

namespace git
{

class Remote final
{
private:
    git_remote* _remote;

public:
    using Ptr = std::shared_ptr<Remote>;

    Remote(git_remote* remote) :
        _remote(remote)
    {}

    ~Remote()
    {
        git_remote_free(_remote);
    }

    void fetch()
    {
        if (!_remote)
        {
            throw GitException("Not a valid remote");
        }

        auto url = wxURI(git_remote_url(_remote));

        git_fetch_options options = GIT_FETCH_OPTIONS_INIT;

        // We will be asked for credentials when the server asks libgit
        options.callbacks.credentials = AcquireCredentials;
        options.callbacks.payload = this;

        auto remoteName = git_remote_name(_remote);

        rMessage() << "Fetching from remote " << remoteName << std::endl;

        auto error = git_remote_fetch(_remote, nullptr, &options, "fetch");
        GitException::ThrowOnError(error);

        rMessage() << "Fetch complete" << std::endl;
    }

    void push(const Reference& ref)
    {
        git_push_options options = GIT_PUSH_OPTIONS_INIT;
        auto refName = ref.getName();
        char* refNamePtr = refName.data();

        const git_strarray refspecs = {
            &refNamePtr,
            1
        };

        auto url = wxURI(git_remote_url(_remote));

        // We will be asked for credentials when the server asks libgit
        options.callbacks.credentials = AcquireCredentials;
        options.callbacks.payload = this;

        auto remoteName = git_remote_name(_remote);
        rMessage() << "Pushing to remote " << remoteName << std::endl;

        auto error = git_remote_push(_remote, &refspecs, &options);
        GitException::ThrowOnError(error);

        rMessage() << "Push complete" << std::endl;
    }

    static Ptr CreateFromName(Repository& repository, const std::string& name)
    {
        git_remote* remote;
        auto error = git_remote_lookup(&remote, repository._get(), name.c_str());

        GitException::ThrowOnError(error);

        return std::make_shared<Remote>(remote);
    }

private:

// Compatibility hack: In Ubuntu 20 we only have older libgit2 versions,
// where git_credential was still called git_cred, map them
#if LIBGIT2_VER_MAJOR < 1
#define git_credential git_cred
#define git_credential_userpass_plaintext_new git_cred_userpass_plaintext_new
#endif

    static int AcquireCredentials(git_cred** out, const char* url, const char* username_from_url, unsigned int allowed_types, void* payload)
    {
        *out = GetCredentialsForRemote(url);
        return *out == nullptr ? GIT_PASSTHROUGH : 0;
    }

    static git_credential* GetCredentialsForRemote(const std::string& remoteUrl)
    {
        wxURI uri(remoteUrl);

        // Create the git:scheme://server string to query the credential manager
        auto credentialResource = fmt::format("git:{0}://{1}", uri.GetScheme().ToStdString(), uri.GetServer().ToStdString());

        auto userAndPass = CredentialManager::RetrievePassword(credentialResource);

        if (!userAndPass.first.empty() && !userAndPass.second.empty())
        {
            rMessage() << "Found credentials for resource " << credentialResource << " in the credential store" << std::endl;

            git_credential* credentials = nullptr;

            auto error = git_credential_userpass_plaintext_new(&credentials, userAndPass.first.c_str(), userAndPass.second.c_str());
            GitException::ThrowOnError(error);

            return credentials;
        }

        return nullptr;
    }
};

}

}
