#include <api/client.h>

#include <core/net/error.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/response.h>
#include <QVariantMap>

namespace http = core::net::http;
namespace net = core::net;

using namespace api;
using namespace std;

Client::Client(Config::Ptr config) :
    config_(config), cancelled_(false) {
}


void Client::get(const net::Uri::Path &path,
                 const net::Uri::QueryParameters &parameters, QJsonDocument &root) {
    // Create a new HTTP client
    auto client = http::make_client();

    // Start building the request configuration
    http::Request::Configuration configuration;

    // Build the URI from its components
    net::Uri uri = net::make_uri(config_->apiroot, path, parameters);
    configuration.uri = client->uri_to_string(uri);

    // Give out a user agent string
    configuration.header.add("User-Agent", config_->user_agent);

    // Build a HTTP request object from our configuration
    auto request = client->head(configuration);

    try {
        // Synchronously make the HTTP request
        // We bind the cancellable callback to #progress_report
        auto response = request->execute(
                    bind(&Client::progress_report, this, placeholders::_1));

        // Check that we got a sensible HTTP status code
        if (response.status != http::Status::ok) {
            throw domain_error(response.body);
        }
        // Parse the JSON from the response
        root = QJsonDocument::fromJson(response.body.c_str());

        // Open weather map API error code can either be a string or int
        QVariant cod = root.toVariant().toMap()["cod"];
        if ((cod.canConvert<QString>() && cod.toString() != "200")
                || (cod.canConvert<unsigned int>() && cod.toUInt() != 200)) {
            throw domain_error(root.toVariant().toMap()["message"].toString().toStdString());
        }
    } catch (net::Error &) {
    }
}

Client::UserRes Client::users(const string& query) {
    // This is the method that we will call from the Query class.
    // It connects to an HTTP source and returns the results.


    // In this case we are going to retrieve JSON data.
    QJsonDocument root;

    // Build a URI and get the contents.
    // The fist parameter forms the path part of the URI.
    // The second parameter forms the CGI parameters.
    get(
    { "search", "users" },
    { { "q", query } },
                root);
    // e.g. http://api.openweathermap.org/data/2.5/weather?q=QUERY&units=metric

    UserRes result;

    // Read out the city we found
    QVariantMap variant = root.toVariant().toMap();
    result.total_count = variant["total_count"].toUInt();

    // Read the Users
    QVariantList items = variant["items"].toList();//.first().toMap();
    for (const QVariant &i : items) {
        QVariantMap item = i.toMap();
        result.users.emplace_back(
                    User {
                        item["id"].toUInt(),
                        item["avatar_url"].toString().toStdString(),
                        item["html_url"].toString().toStdString(),
                        item["followers_url"].toString().toStdString(),
                        item["following_url"].toString().toStdString(),
                        item["gists_url"].toString().toStdString(),
                        item["starred_url"].toString().toStdString(),
                        item["organizations_url"].toString().toStdString(),
                        item["repos_url"].toString().toStdString(),
                        item["name"].toString().toStdString(),
                        item["company"].toString().toStdString(),
                        item["blog"].toString().toStdString(),
                        item["location"].toString().toStdString(),
                        item["email"].toString().toStdString(),
                        item["hireable"].toBool(),
                        item["bio"].toString().toStdString(),
                        item["public_repos"].toUInt(),
                        item["public_gists"].toUInt(),
                        item["followers"].toUInt(),
                        item["following"].toUInt()
                    }
                    );
    }
    return result;
}

Client::RepositoryRes Client::repositories(const string& query) {
    // This is the method that we will call from the Query class.
    // It connects to an HTTP source and returns the results.


    // In this case we are going to retrieve JSON data.
    QJsonDocument root;

    // Build a URI and get the contents.
    // The fist parameter forms the path part of the URI.
    // The second parameter forms the CGI parameters.
    get(
    { "search", "repositories" },
    { { "q", query } },
                root);
    // e.g. http://api.openweathermap.org/data/2.5/weather?q=QUERY&units=metric

    RepositoryRes result;

    // Read out the city we found
    QVariantMap variant = root.toVariant().toMap();
    result.total_count = variant["total_count"].toUInt();

    // Read the Users
    QVariantList items = variant["items"].toList();
    for (const QVariant &i : items) {
        QVariantMap item = i.toMap();
        QVariantMap owner = item["owner"].toMap();
        result.repositories.emplace_back(
                    Repository {
                        Owner {
                            owner["login"].toString().toStdString(),
                            owner["id"].toUInt(),
                            owner["avatar_url"].toString().toStdString(),
                            owner["html_url"].toString().toStdString()
                        },
                        item["name"].toString().toStdString(),
                        item["full_name"].toString().toStdString(),
                        item["description"].toString().toStdString(),
                        item["private"].toBool(),
                        item["fork"].toBool(),
                        item["html_url"].toString().toStdString(),
                        item["language"].toString().toStdString(),
                        item["forks_count"].toUInt(),
                        item["stargazers_count"].toUInt(),
                        item["watchers_count"].toUInt(),
                        item["open_issues_count"].toUInt(),
                        item["created_at"].toString().toStdString(),
                        item["pushed_at"].toString().toStdString()
                    }
                    );
    }
    return result;
}

Client::CodeRes Client::code(const string &query, const string &repo = NULL)
{
    // This is the method that we will call from the Query class.
    // It connects to an HTTP source and returns the results.

    // In this case we are going to retrieve JSON data.
    QJsonDocument root;

    // Build a URI and get the contents.
    // The fist parameter forms the path part of the URI.
    // The second parameter forms the CGI parameters.
    get(
    { "search", "code" },
    { { "q", query + "+repo:" + repo } },
                root);
    // e.g. http://api.openweathermap.org/data/2.5/weather?q=QUERY&units=metric

    CodeRes result;

    // Read out the city we found
    QVariantMap variant = root.toVariant().toMap();
    result.total_count = variant["total_count"].toUInt();

    // Read the Users
    QVariantList items = variant["items"].toList();
    for (const QVariant &i : items) {
        QVariantMap item = i.toMap();
        QVariantMap repository = item["repository"].toMap();
        QVariantMap owner = repository["owner"].toMap();
        result.codes.emplace_back(
                    Code {
                        item["name"].toString().toStdString(),
                        item["path"].toString().toStdString(),
                        item["html_url"].toString().toStdString(),
                        Repository {
                            Owner {
                                owner["login"].toString().toStdString(),
                                owner["id"].toUInt(),
                                owner["avatar_url"].toString().toStdString(),
                                owner["html_url"].toString().toStdString()
                            },
                            repository["name"].toString().toStdString(),
                            repository["full_name"].toString().toStdString(),
                            repository["description"].toString().toStdString(),
                            repository["private"].toBool(),
                            repository["fork"].toBool(),
                            repository["html_url"].toString().toStdString(),
                            repository["language"].toString().toStdString(),
                            repository["forks_count"].toUInt(),
                            repository["stargazers_count"].toUInt(),
                            repository["watchers_count"].toUInt(),
                            repository["open_issues_count"].toUInt(),
                            repository["created_at"].toString().toStdString(),
                            repository["pushed_at"].toString().toStdString()
                        }
                    }
                    );
    }
    return result;
}

http::Request::Progress::Next Client::progress_report(
        const http::Request::Progress&) {

    return cancelled_ ?
                http::Request::Progress::Next::abort_operation :
                http::Request::Progress::Next::continue_operation;
}

std::string Client::getRepo() const
{
    return repo;
}

void Client::setRepo(const std::string &value)
{
    repo = value;
}


void Client::cancel() {
    cancelled_ = true;
}

Config::Ptr Client::config() {
    return config_;
}

