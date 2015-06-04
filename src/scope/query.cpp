#include <boost/algorithm/string/trim.hpp>

#include <scope/localization.h>
#include <scope/query.h>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>

#include <iomanip>
#include <sstream>

namespace sc = unity::scopes;
namespace alg = boost::algorithm;

using namespace std;
using namespace api;
using namespace scope;

/**
 * Define the larger "current weather" layout.
 *
 * The icons are larger.
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

Query::Query(const sc::CannedQuery &query, const sc::SearchMetadata &metadata,
             Config::Ptr config) :
    sc::SearchQueryBase(query, metadata), client_(config) {
}

void Query::cancelled() {
    client_.cancel();
}


void Query::run(sc::SearchReplyProxy const& reply) {
    try {
        // Start by getting information about the query
        const sc::CannedQuery &query(sc::SearchQueryBase::query());

        // Trim the query string of whitespace
        string query_string = alg::trim_copy(query.query_string());

        // the Client is the helper class that provides the results
        // without mixing APIs and scopes code.
        // Add your code to retreive xml, json, or any other kind of result
        // in the client.
        Client::RepositoryRes repositories;
        if (query_string.empty()) {
            // If the string is empty, get the current weather for London
            repositories = client_.repositories("ubuntu-touch");
        } else {
            // otherwise, get the current weather for the search string
            repositories = client_.repositories(query_string);
        }

        // Build up the description for the city
        //stringstream ss(stringstream::in | stringstream::out);
        //ss << current.city.name << ", " << current.city.country;

        // Register a category for the current weather, with the title we just built
        auto repositories_cat = reply->register_category("repositories", "", "",
                                                     sc::CategoryRenderer(REPOSITORY_TEMPLATE));

        for (const auto &repository : repositories.repositories) {
            // Iterate over the trackslist
            sc::CategorisedResult res(repositories_cat);

            // We must have a URI
            res.set_uri(repository.html_url);

            // We also need the track title
            res.set_title(repository.full_name);

            // Set the rest of the attributes, art, artist, etc
            res.set_art(repository.owner.avatar_url);
            res["description"] = repository.description;
            res["developer_uri"] = repository.owner.url;

            // Push the result
            if (!reply->push(res)) {
                // If we fail to push, it means the query has been cancelled.
                // So don't continue;
                return;
            }
        }
    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}

