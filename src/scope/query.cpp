#include <boost/algorithm/string/trim.hpp>

#include <scope/localization.h>
#include <scope/query.h>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/Department.h>

#include <QDate>

#include <iomanip>
#include <sstream>

namespace sc = unity::scopes;
namespace alg = boost::algorithm;

using namespace std;
using namespace api;
using namespace scope;

/**
 * Repository result template
 */
const static string REPOSITORY_TEMPLATE =
        R"(
{
        "schema-version": 1,
        "template": {
        "category-layout": "grid",
        "card-size": "medium",
        "overlay": true
        },
        "components": {
        "title": "title",
        "art" : {
        "field": "art"
        },
        "overlay-color": "overlay"
        }
        }
        )";

/**
 * Code result remplate
 */
const static string CODE_TEMPLATE =
        R"(
{
        "schema-version": 1,
        "template": {
        "category-layout": "vertical-journal",
        "card-size": 32
        },
        "components": {
        "title": "title",
        "summary":"summary",
        "type":"type"
        }
        }
        )";

/**
 * 404 page - Nothing return from the query
 */
const static string EMPTY_TEMPLATE =
        R"(
{
        "schema-version": 1,
        "template": {
        "category-layout": "grid",
        "card-size": "large"
        },
        "components": {
        "title": "title",
        "summary": "summary",
        "type": "type"
        }
        }
        )";

Query::Query(const sc::CannedQuery &query, const sc::SearchMetadata &metadata,
             Config::Ptr config) :
    sc::SearchQueryBase(query, metadata), client_(config) {
}

void Query::cancelled() {
    client_.cancel();
}


void Query::run(sc::SearchReplyProxy const& reply) {
    try {
        initScope();

        // Start by getting information about the query
        const sc::CannedQuery &query(sc::SearchQueryBase::query());

        // Trim the query string of whitespace
        string query_string = alg::trim_copy(query.query_string());

        // Create the root department with an empty string for the 'id' parameter (the first one)
        sc::Department::SPtr all_depts = sc::Department::create("", query, "Repositories");
client_.setRepo("jquery/jquery");
        // Create new departments
        sc::Department::SPtr code_department;
        if(client_.getRepo().empty())
            code_department = sc::Department::create("code", query, "Code");
        else
            code_department = sc::Department::create("code", query, "Code in " + client_.getRepo());

        // Register them as subdepartments of the root
        all_depts->set_subdepartments({code_department});

        // Register the root department on the reply
        reply->register_departments(all_depts);

        // the Client is the helper class that provides the results
        // without mixing APIs and scopes code.
        // Add your code to retreive xml, json, or any other kind of result
        // in the client.
        Client::RepositoryRes repositories;
        Client::CodeRes codes;

        if (query_string.empty()) {
            // If the string is empty, get the current weather for London
            if(query.department_id() == "") // Root department
                repositories = client_.repositories("ubuntu-touch");
            else if(query.department_id() == "code")
                codes = client_.code("addClass", s_repo);
        } else {
            // otherwise, get the current weather for the search string
            if(query.department_id() == "") // Root department
                repositories = client_.repositories(query_string);
            else if(query.department_id() == "code")
                codes = client_.code(query_string, s_repo);
        }

        // Build up the description for the city
        //stringstream ss(stringstream::in | stringstream::out);
        //ss << current.city.name << ", " << current.city.country;
        std::cout << query.department_id() << std::endl;
        /**
          * 404 error
          */
        if((repositories.total_count <= 0 && query.department_id() == "")
                || (codes.total_count <= 0 && query.department_id() == "code")) {
            auto empty_cat = reply->register_category("empty",
                                                      _("Nothing found"), "", sc::CategoryRenderer(EMPTY_TEMPLATE));

            // Create a result
            sc::CategorisedResult res(empty_cat);

            // Set informations
            res.set_uri("-1");
            res.set_title("Nothing here");
            res["summary"] = "I couldn't find any result. Please, check your connectivity and try again.";
            res["description"] = "No results found";
            res["type"] = "empty";

            // Push the result
            if (!reply->push(res)) {
                // If we fail to push, it means the query has been cancelled.
                // So don't continue;
                return;
            }
        }
        /**
         * Repository found
         */
        else if(query.department_id() == "") {
            // Register a category for the current weather, with the title we just built
            auto repositories_cat = reply->register_category("repositories", _("Repositories"), "",
                                                             sc::CategoryRenderer(REPOSITORY_TEMPLATE));
            auto code_cat = reply->register_category("code", _("Code"), "",
                                                     sc::CategoryRenderer(CODE_TEMPLATE));

            for (const auto &repository : repositories.repositories) {
                // Iterate over the trackslist
                sc::CategorisedResult res(repositories_cat);

                // We must have a URI
                res.set_uri(repository.html_url);

                // We also need the track title
                res.set_title(repository.full_name);

                // Set the rest of the attributes, art, artist, etc
                res.set_art(repository.owner.avatar_url);

                QDate createdDate = QDate::fromString(QString::fromStdString(repository.created_at), Qt::ISODate);
                QDate pushedDate = QDate::fromString(QString::fromStdString(repository.pushed_at), Qt::ISODate);
                res["description"] = repository.description + "\n\nLanguage: " + repository.language +
                        "\n\n" + toStr(repository.stargazers_count) + " stargazers, " +
                        toStr(repository.watchers_count) + " watchers." +
                        "\n\nCreated at " + createdDate.toString().toStdString() +
                        "\nLast push " + pushedDate.toString().toStdString() +
                        ", " + toStr(pushedDate.daysTo(QDate::currentDate())) +
                        " days ago." +
                        "\n\nOpen issues: " + toStr(repository.open_issues_count);
                res["developer_uri"] = repository.owner.url;
                res["new_issue_uri"] = repository.html_url + "/issues/new";
                res["type"] = "repository";

                sc::CategorisedResult codeRes(code_cat);
                sc::CannedQuery code_query(query);
                code_query.set_department_id("code");
                codeRes.set_uri(code_query.to_uri());
                res["code_query"] = codeRes.uri();
                client_.setRepo(repository.full_name);

                // Push the result
                if (!reply->push(res)) {
                    // If we fail to push, it means the query has been cancelled.
                    // So don't continue;
                    return;
                }
            }
        }
        /**
          * Code found
          */
        else if(query.department_id() == "code") {
            // Register a category for the current weather, with the title we just built
            auto code_cat = reply->register_category("code", _("Code in ") + client_.getRepo(), "",
                                                     sc::CategoryRenderer(CODE_TEMPLATE));

            for (const auto &code : codes.codes) {
                // Iterate over the trackslist
                sc::CategorisedResult res(code_cat);

                // We must have a URI
                res.set_uri(code.html_url);

                // We also need the track title
                res.set_title(code.name);
                res["description"] = code.repository.full_name;
                res["developer_uri"] = code.repository.owner.url;
                res["new_issue_uri"] = code.repository.html_url + "/issues/new";
                res["type"] = "code";

                // Push the result
                if (!reply->push(res)) {
                    // If we fail to push, it means the query has been cancelled.
                    // So don't continue;
                    return;
                }
            }
        }
    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}

std::string Query::toStr(const int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

void Query::initScope()
{
    unity::scopes::VariantMap config = settings();
    if (config.empty())
        cerr << "CONFIG EMPTY!" << endl;

    s_repo = config["repo"].get_string();
}
