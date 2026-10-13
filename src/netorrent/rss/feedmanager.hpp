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
    // Posted (with a human-readable status in the event's string) after a feed
    // has been fetched and processed, so the UI can show progress.
    wxDECLARE_EVENT(ptEVT_RSS_UPDATED, wxCommandEvent);

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
        void RefreshFeed(Feed const& feed);

        std::vector<Feed> GetFeeds();
        void SaveFeed(Feed& feed);
        void RemoveFeed(int id);

        // Result of the most recent feed fetch, for display in the UI.
        std::string GetLastStatus() const { return m_lastStatus; }

        static constexpr int PollIntervalSeconds = 900;

    private:
        void OnTimer(wxTimerEvent&);
        void Fetch(Feed const& feed);
        void HandleFeedBody(Feed const& feed, int statusCode, std::string const& body);
        void ProcessItem(Feed const& feed, wxXmlNode* item, int& items, int& matched);
        void ResolveAndAdd(Feed const& feed, std::string const& link, std::string const& guid);
        void AddTorrent(Feed const& feed, std::string const& link, std::string const& guid);
        bool PassesFilter(Feed const& feed, std::string const& text) const;
        bool MarkSeen(int feedId, std::string const& guid, std::string const& title);
        void UnmarkSeen(int feedId, std::string const& guid);
        std::string SavePath();
        void SetStatus(std::string const& status);

        static bool IsTorrentUrl(std::string const& link);
        static std::string ExtractTorrentLink(std::string const& html);

        std::shared_ptr<Core::Database> m_db;
        std::shared_ptr<Core::Configuration> m_cfg;
        BitTorrent::Session* m_session;
        std::unique_ptr<Http::HttpClient> m_httpClient;
        wxTimer m_timer;
        std::string m_lastStatus;
    };
}
