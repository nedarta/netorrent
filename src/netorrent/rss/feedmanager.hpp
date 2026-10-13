#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/timer.h>

#include <memory>
#include <string>
#include <vector>

namespace pt::Core { class Configuration; class Database; }
namespace pt::BitTorrent { class Session; }
namespace pt::Http { class HttpClient; }
namespace pt::RSS { struct Feed; }

class wxXmlNode;

namespace pt::RSS
{
    // Periodically fetches the configured RSS/Atom feeds, applies each feed's
    // include/exclude filter to the item titles, and adds matching magnet links
    // or .torrent URLs to the session. Items are de-duplicated per feed.
    class FeedManager : public wxEvtHandler
    {
    public:
        FeedManager(std::shared_ptr<Core::Database> db, std::shared_ptr<Core::Configuration> cfg);
        virtual ~FeedManager();

        void SetSession(BitTorrent::Session* session);
        void Start();
        void RefreshAll();

        std::vector<Feed> GetFeeds();
        void SaveFeed(Feed& feed);
        void RemoveFeed(int id);

        static constexpr int PollIntervalSeconds = 900;

    private:
        void OnTimer(wxTimerEvent&);
        void Fetch(Feed const& feed);
        void HandleFeedBody(Feed const& feed, int statusCode, std::string const& body);
        void ProcessItem(Feed const& feed, wxXmlNode* item);
        void AddTorrent(Feed const& feed, std::string const& link);
        bool PassesFilter(Feed const& feed, std::string const& text) const;
        bool MarkSeen(int feedId, std::string const& guid, std::string const& title);

        std::shared_ptr<Core::Database> m_db;
        std::shared_ptr<Core::Configuration> m_cfg;
        BitTorrent::Session* m_session;
        std::unique_ptr<Http::HttpClient> m_httpClient;
        wxTimer m_timer;
    };
}
