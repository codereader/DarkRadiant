#pragma once

#include <git2.h>
#include "Repository.h"
#include "CredentialManager.h"
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
            rError() << "Not a valid remote" << std::endl;
            return;
        }

        auto url = wxURI(git_remote_url(_remote));

        git_fetch_options options;
        git_fetch_options_init(&options, GIT_FETCH_OPTIONS_VERSION);

        // Create the git:scheme://server string to query the credential manager
        auto credentialResource = fmt::format("git:{0}://{1}", url.GetScheme().ToStdString(), url.GetServer().ToStdString());

        auto userAndPass = CredentialManager::RetrievePassword(credentialResource);

        if (!userAndPass.first.empty() && !userAndPass.second.empty())
        {
            rMessage() << "Found credentials for resource " << credentialResource << " in the credential store" << std::endl;

            git_credential* credentials = nullptr;

            if (git_credential_userpass_plaintext_new(&credentials, userAndPass.first.c_str(), userAndPass.second.c_str()) < 0)
            {
                rError() << "Unable to create credentials" << std::endl;
                return;
            }

            options.callbacks.credentials = AcquireCredentials;
            options.callbacks.payload = credentials;
        }

        auto remoteName = git_remote_name(_remote);

        rMessage() << "Fetching from remote " << remoteName << std::endl;

        if (git_remote_fetch(_remote, nullptr, &options, "fetch") < 0)
        {
            const auto* error = git_error_last();

            rError() << "Fetch error " << error->message << std::endl;
            return;
        }

        rMessage() << "Fetch complete";
    }

    static Ptr CreateFromName(Repository& repository, const std::string& name)
    {
        git_remote* remote;

        if (git_remote_lookup(&remote, repository._get(), name.c_str()) < 0)
        {
            rWarning() << "Failed to look up the remote " << name << std::endl;
            return Ptr();
        }

        return std::make_shared<Remote>(remote);
    }

private:
    static int AcquireCredentials(git_cred** out, const char* url, const char* username_from_url, unsigned int allowed_types, void* payload)
    {
        *out = reinterpret_cast<git_credential*>(payload);
        return 0;
    }
};

}

}
