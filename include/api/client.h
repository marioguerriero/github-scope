#ifndef API_CLIENT_H_
#define API_CLIENT_H_

#include <api/config.h>

#include <atomic>
#include <deque>
#include <map>
#include <string>
#include <core/net/http/request.h>
#include <core/net/uri.h>

#include <QJsonDocument>

namespace api {

/**
 * Provide a nice way to access the HTTP API.
 *
 * We don't want our scope's code to be mixed together with HTTP and JSON handling.
 */
class Client {
public:

    /**
      * Information about a Owner
      */
    struct Owner {
        std::string login;
        unsigned int id;
        std::string avatar_url;
        std::string url;
    };

    /**
      * Information about a User
      */
    struct User {
        unsigned int id;
        std::string avatar_url;
        std::string html_url;
        std::string followers_url;
        std::string following_url;
        std::string gists_url;
        std::string starred_url;
        std::string organizations_url;
        std::string repos_url;
        std::string name;
        std::string company;
        std::string blog;
        std::string location;
        std::string email;
        bool hireable;
        std::string bio;
        unsigned int public_repos;
        unsigned int public_gists;
        unsigned int followers;
        unsigned int following;
    };

    /**
      * List of Users
      */
    typedef std::deque<User> UserList;

    /**
      * A User searching result
      */
    struct UserRes {
        int total_count;
        UserList users;
    };

    /**
      * Information about a repository
      */
    struct Repository {
        Owner owner;
        std::string name;
        std::string full_name;
        std::string description;
        bool prvt;
        bool fork;
        std::string html_url;
        std::string language;
        unsigned int forks_count;
        unsigned int stargazers_count;
        unsigned int watchers_count;
        unsigned int open_issues_count;
        std::string created_at;
        std::string pushed_at;
    };

    /**
      * List of Repositories
      */
    typedef std::deque<Repository> RepositoryList;

    /**
      * A Repository searching result
      */
    struct RepositoryRes {
        unsigned int total_count;
        RepositoryList repositories;
    };

    Client(Config::Ptr config);

    virtual ~Client() = default;

    /**
     * Search for users
     */
    virtual UserRes users(const std::string &query);

    /**
     * Search for repositories
     */
    virtual RepositoryRes repositories(const std::string &query);

    /**
     * Cancel any pending queries (this method can be called from a different thread)
     */
    virtual void cancel();

    virtual Config::Ptr config();

protected:
    void get(const core::net::Uri::Path &path,
             const core::net::Uri::QueryParameters &parameters,
             QJsonDocument &root);
    /**
     * Progress callback that allows the query to cancel pending HTTP requests.
     */
    core::net::http::Request::Progress::Next progress_report(
            const core::net::http::Request::Progress& progress);

    /**
     * Hang onto the configuration information
     */
    Config::Ptr config_;

    /**
     * Thread-safe cancelled flag
     */
    std::atomic<bool> cancelled_;
};

}

#endif // API_CLIENT_H_

