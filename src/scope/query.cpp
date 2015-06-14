#include <boost/algorithm/string/trim.hpp>

#include <scope/localization.h>
#include <scope/query.h>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/Department.h>
#include <unity/scopes/OptionSelectorFilter.h>

#include <QDate>
#include <QSettings>

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
        loadCache();

        // Start by getting information about the query
        const sc::CannedQuery &query(sc::SearchQueryBase::query());

        // Trim the query string of whitespace
        string query_string = alg::trim_copy(query.query_string());

        /*std::string s_homepage;
        sc::Filters filters;
        sc::OptionSelectorFilter::SPtr optionsFilter = sc::OptionSelectorFilter::create("category", s_homepage);
        optionsFilter->set_display_hints(1);
        optionsFilter->add_option("movie", _("Movies"));
        optionsFilter->add_option("tv", _("TV-series"));
        optionsFilter->active_options(query.filter_state());
        filters.push_back(optionsFilter);
        reply->push(filters, query.filter_state());

        // Create the root department with an empty string for the 'id' parameter (the first one)
        sc::Department::SPtr all_depts = sc::Department::create("", query, "Repositories");

        // Create new departments
        sc::Department::SPtr code_department;
        code_department = sc::Department::create("code", query, "Code in " + c_repo);

        // Register them as subdepartments of the root
        all_depts->set_subdepartments({code_department});

        // Register the root department on the reply
        reply->register_departments(all_depts);
*/
        // the Client is the helper class that provides the results
        // without mixing APIs and scopes code.
        // Add your code to retreive xml, json, or any other kind of result
        // in the client.
        Client::RepositoryRes repositories;
        Client::CodeRes codes;

        // Reset cached informations if users does not want them to be saved
        /*if(!s_save) {
            c_query = "ubuntu-touch";
            c_repo = "torvalds/linux";
        }*/

        if (query_string.empty()) {
            // If the string is empty, get the current weather for London
            //if(query.department_id() == "")
                repositories = client_.repositories(c_query, s_name, s_description, s_readme);
            //else if(query.department_id() == "code")
            //    codes = client_.code(c_query, c_repo);
        } else {
            // otherwise, get the current weather for the search string
            //if(query.department_id() == "") {
                repositories = client_.repositories(query_string, s_name, s_description, s_readme);
            //    c_query = query_string;
            //}
            //else if(query.department_id() == "code")
            //    codes = client_.code(query_string, c_repo);
            // Update cached query
            c_query = query_string;
        }

        // Build up the description for the city
        //stringstream ss(stringstream::in | stringstream::out);
        //ss << current.city.name << ", " << current.city.country;

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
            res["summary"] = "I couldn't find any result. Please, change you query or check your connectivity and try again.";
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

                //sc::CategorisedResult codeRes(code_cat);
                //sc::CannedQuery code_query(query);
                //code_query.set_department_id("code");
                //codeRes.set_uri(code_query.to_uri());
                res["code_query"] = repository.html_url + "/search";
                //c_repo = repository.full_name;

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
            auto code_cat = reply->register_category("code", _("Code in ") + c_repo, "",
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
        // Update cache
        updateCache();
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

    s_name = config["searchName"].get_bool();
    s_description = config["searchDescription"].get_bool();
    s_readme= config["searchReadme"].get_bool();
}

std::string Query::getCachePath() const
{
    return cachePath;
}

void Query::setCachePath(const std::string &value)
{
    cachePath = value;
}

void Query::loadCache()
{
    QSettings cache(QString::fromUtf8(cachePath.c_str()), QSettings::NativeFormat);
    c_query = cache.value("query", "module").toString().toStdString();
    c_repo = cache.value("repo", "linux/linux").toString().toStdString();
}

void Query::updateCache()
{
    QSettings cache(QString::fromUtf8(cachePath.c_str()), QSettings::NativeFormat);
    cache.setValue("query", QVariant(c_query.c_str()));
    cache.setValue("repo", QVariant(c_repo.c_str()));
}
