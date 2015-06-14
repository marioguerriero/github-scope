#ifndef SCOPE_QUERY_H_
#define SCOPE_QUERY_H_

#include <api/client.h>

#include <unity/scopes/SearchQueryBase.h>
#include <unity/scopes/ReplyProxyFwd.h>

namespace scope {

/**
 * Represents an individual query.
 *
 * A new Query object will be constructed for each query. It is
 * given query information, metadata about the search, and
 * some scope-specific configuration.
 */
class Query: public unity::scopes::SearchQueryBase {
public:
    Query(const unity::scopes::CannedQuery &query,
          const unity::scopes::SearchMetadata &metadata, api::Config::Ptr config);

    ~Query() = default;

    void cancelled() override;

    void run(const unity::scopes::SearchReplyProxy &reply) override;

    std::string getCachePath() const;
    void setCachePath(const std::string &value);

private:
    api::Client client_;

    std::string toStr(const int value);

    // Settings
    void initScope();
    bool s_name;
    bool s_description;
    bool s_readme;

    // Cache informations
    std::string cachePath;
    std::string c_query;
    std::string c_repo;
    void loadCache();
    void updateCache();
};

}

#endif // SCOPE_QUERY_H_


