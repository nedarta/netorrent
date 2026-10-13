#include "feedmanager.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <regex>

#include <boost/log/trivial.hpp>
#include <fmt/format.h>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/load_torrent.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <wx/sstream.h>
#include <wx/xml/xml.h>

#include "../bittorrent/session.hpp"
#include "../core/configuration.hpp"
#include "../core/database.hpp"
#include "../http/httpclient.hpp"
#include "rssfeed.hpp"

using pt::RSS::Feed;
using pt::RSS::FeedManager;

wxDEFINE_EVENT(pt::RSS::ptEVT_RSS_UPDATED, wxCommandEvent);

namespace
{
    std::string ToLower(std::string s)
    {
        std::transform(
            s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        return s;
    }

    wxString ChildText(wxXmlNode* node, wxString const& name)
    {
        for (wxXmlNode* child = node->GetChildren(); child != nullptr; child = child->GetNext())
        {
            if (child->GetName() == name)
            {
                return child->GetNodeContent();
            }
        }

        return wxEmptyString;
    }

    wxString ChildAttribute(wxXmlNode* node, wxString const& name, wxString const& attribute)
    {
        for (wxXmlNode* child = node->GetChildren(); child != nullptr; child = child->GetNext())
        {
            if (child->GetName() == name)
            {
                wxString value;
                if (child->GetAttribute(attribute, &value))
                {
                    return value;
                }
            }
        }

        return wxEmptyString;
    }
}

FeedManager::FeedManager(std::shared_ptr<pt::Core::Database> db, std::shared_ptr<pt::Core::Configuration> cfg)
    : m_db(db),
    m_cfg(cfg),
    m_session(nullptr),
    m_httpClient(std::make_unique<pt::Http::HttpClient>()),
    m_timer(this)
{
    this->Bind(wxEVT_TIMER, &FeedManager::OnTimer, this);
}

FeedManager::~FeedManager()
{
    m_timer.Stop();
}

void FeedManager::SetSession(pt::BitTorrent::Session* session)
{
    m_session = session;
}

void FeedManager::Start()
{
    m_timer.Start(PollIntervalSeconds * 1000);
    RefreshAll();
}

void FeedManager::OnTimer(wxTimerEvent&)
{
    RefreshAll();
}

void FeedManager::SetStatus(std::string const& status)
{
    m_lastStatus = status;
    BOOST_LOG_TRIVIAL(info) << "RSS: " << status;

    auto* evt = new wxCommandEvent(ptEVT_RSS_UPDATED);
    evt->SetString(wxString::FromUTF8(status));
    wxQueueEvent(this, evt);
}

void FeedManager::RefreshAll()
{
    for (Feed const& feed : GetFeeds())
    {
        if (feed.enabled)
        {
            Fetch(feed);
        }
    }
}

void FeedManager::RefreshFeed(Feed const& feed)
{
    if (feed.enabled)
    {
        Fetch(feed);
    }
}

void FeedManager::Fetch(Feed const& feed)
{
    if (feed.url.empty())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(info) << "RSS: fetching '" << feed.name << "' (" << feed.url << ")";

    m_httpClient->Get(
        wxString::FromUTF8(feed.url),
        [this, feed](int statusCode, std::string const& body)
        {
            HandleFeedBody(feed, statusCode, body);
        });
}

void FeedManager::HandleFeedBody(Feed const& feed, int statusCode, std::string const& body)
{
    if (statusCode != 200 || body.empty())
    {
        SetStatus(fmt::format("{}: request failed (status {})", feed.name, statusCode));
        return;
    }

    wxStringInputStream stream(wxString::FromUTF8(body));
    wxXmlDocument doc;

    if (!doc.Load(stream))
    {
        SetStatus(fmt::format("{}: could not parse the feed", feed.name));
        return;
    }

    wxXmlNode* root = doc.GetRoot();

    if (root == nullptr)
    {
        return;
    }

    int items = 0;
    int matched = 0;
    wxString const rootName = root->GetName();

    if (rootName == "rss")
    {
        // RSS 2.0: rss > channel > item*
        for (wxXmlNode* child = root->GetChildren(); child != nullptr; child = child->GetNext())
        {
            if (child->GetName() != "channel")
            {
                continue;
            }

            for (wxXmlNode* item = child->GetChildren(); item != nullptr; item = item->GetNext())
            {
                if (item->GetName() == "item")
                {
                    ProcessItem(feed, item, items, matched);
                }
            }

            break;
        }
    }
    else if (rootName == "feed")
    {
        // Atom: feed > entry*
        for (wxXmlNode* item = root->GetChildren(); item != nullptr; item = item->GetNext())
        {
            if (item->GetName() == "entry")
            {
                ProcessItem(feed, item, items, matched);
            }
        }
    }
    else
    {
        SetStatus(fmt::format("{}: unsupported feed format ({})", feed.name, rootName.ToStdString()));
        return;
    }

    SetStatus(fmt::format("{}: {} items, {} matched", feed.name, items, matched));
}

void FeedManager::ProcessItem(Feed const& feed, wxXmlNode* item, int& items, int& matched)
{
    ++items;

    wxString title = ChildText(item, "title");
    wxString guid = ChildText(item, "guid");

    if (guid.empty())
    {
        guid = ChildText(item, "id"); // Atom
    }

    // Link: RSS <link>text</link>, Atom <link href="..."/>
    wxString link = ChildText(item, "link");

    if (link.empty())
    {
        link = ChildAttribute(item, "link", "href");
    }

    // Torrent enclosures take precedence over the item link.
    wxString enclosure = ChildAttribute(item, "enclosure", "url");

    wxString target = !enclosure.empty() ? enclosure : link;

    if (target.empty())
    {
        return;
    }

    if (guid.empty())
    {
        guid = target;
    }

    std::string const titleStd = title.ToStdString();

    if (!PassesFilter(feed, titleStd))
    {
        return;
    }

    if (!MarkSeen(feed.id, guid.ToStdString(), titleStd))
    {
        return; // already processed in an earlier pass
    }

    ++matched;
    BOOST_LOG_TRIVIAL(info) << "RSS: '" << feed.name << "' matched '" << titleStd << "'";
    ResolveAndAdd(feed, target.ToStdString(), guid.ToStdString());
}

void FeedManager::ResolveAndAdd(Feed const& feed, std::string const& link, std::string const& guid)
{
    if (link.rfind("magnet:", 0) == 0 || IsTorrentUrl(link))
    {
        AddTorrent(feed, link, guid);
        return;
    }

    // The link may be an HTML details page, or a URL that serves the .torrent
    // itself (many trackers use an obfuscated download URL). Fetch it and work
    // out which.
    m_httpClient->Get(
        wxString::FromUTF8(link),
        [this, feed, link, guid](int statusCode, std::string const& body)
        {
            if (statusCode != 200 || body.empty())
            {
                BOOST_LOG_TRIVIAL(info) << "RSS: could not open " << link << " (" << statusCode << ")";
                UnmarkSeen(feed.id, guid);
                return;
            }

            // A bencoded body is the .torrent file itself.
            if (body[0] == 'd')
            {
                lt::error_code ec;
                lt::add_torrent_params params = lt::load_torrent_buffer(
                    lt::span<char const>(body.data(), body.size()), ec, lt::load_torrent_limits{});

                if (!ec)
                {
                    params.save_path = SavePath();
                    m_session->AddTorrent(params);
                    BOOST_LOG_TRIVIAL(info) << "RSS: added torrent from '" << feed.name << "' (" << link << ")";
                    return;
                }
            }

            // Otherwise scrape the page for a magnet or .torrent link.
            std::string const resolved = ExtractTorrentLink(body);

            if (resolved.empty())
            {
                BOOST_LOG_TRIVIAL(info) << "RSS: no magnet or .torrent link found on " << link;
                UnmarkSeen(feed.id, guid);
                return;
            }

            AddTorrent(feed, resolved, guid);
        });
}

void FeedManager::AddTorrent(Feed const& feed, std::string const& link, std::string const& guid)
{
    if (m_session == nullptr)
    {
        return;
    }

    std::string const savePath = SavePath();

    if (link.rfind("magnet:", 0) == 0)
    {
        lt::error_code ec;
        lt::add_torrent_params params = lt::parse_magnet_uri(link, ec);

        if (ec)
        {
            BOOST_LOG_TRIVIAL(warning) << "RSS: invalid magnet '" << link << "': " << ec.message();
            UnmarkSeen(feed.id, guid);
            return;
        }

        params.save_path = savePath;
        m_session->AddTorrent(params);
        return;
    }

    // Otherwise fetch the .torrent and add it from the buffer.
    m_httpClient->Get(
        wxString::FromUTF8(link),
        [this, feed, guid, savePath](int statusCode, std::string const& body)
        {
            if (statusCode != 200 || body.empty())
            {
                BOOST_LOG_TRIVIAL(warning) << "RSS: failed to download torrent for '" << feed.name << "' (" << statusCode << ")";
                UnmarkSeen(feed.id, guid);
                return;
            }

            lt::error_code ec;
            lt::add_torrent_params params = lt::load_torrent_buffer(
                lt::span<char const>(body.data(), body.size()), ec, lt::load_torrent_limits{});

            if (ec)
            {
                BOOST_LOG_TRIVIAL(warning) << "RSS: failed to parse torrent for '" << feed.name << "': " << ec.message();
                UnmarkSeen(feed.id, guid);
                return;
            }

            params.save_path = savePath;
            m_session->AddTorrent(params);
        });
}

std::string FeedManager::SavePath()
{
    if (auto sp = m_cfg->Get<std::string>("default_save_path"); sp.has_value())
    {
        return *sp;
    }

    return {};
}

bool FeedManager::PassesFilter(Feed const& feed, std::string const& text) const
{
    auto matches = [&feed, &text](std::string const& pattern) -> bool
    {
        if (pattern.empty())
        {
            return false;
        }

        if (feed.isRegex)
        {
            try
            {
                return std::regex_search(text, std::regex(pattern, std::regex_constants::icase));
            }
            catch (std::regex_error const&)
            {
                return false;
            }
        }

        return ToLower(text).find(ToLower(pattern)) != std::string::npos;
    };

    if (!feed.includeFilter.empty() && !matches(feed.includeFilter))
    {
        return false;
    }

    if (!feed.excludeFilter.empty() && matches(feed.excludeFilter))
    {
        return false;
    }

    return true;
}

bool FeedManager::IsTorrentUrl(std::string const& link)
{
    auto pos = link.find(".torrent");

    return pos != std::string::npos
        && (pos + 8 == link.size() || link[pos + 8] == '?');
}

std::string FeedManager::ExtractTorrentLink(std::string const& html)
{
    static const std::regex magnetRe(R"(magnet:\?xt=[^"'\s<>]+)", std::regex::icase);
    static const std::regex torrentRe(R"((https?://[^"'\s<>]+\.torrent))", std::regex::icase);

    std::smatch match;

    if (std::regex_search(html, match, magnetRe))
    {
        return match.str();
    }

    if (std::regex_search(html, match, torrentRe))
    {
        return match.str();
    }

    return {};
}

bool FeedManager::MarkSeen(int feedId, std::string const& guid, std::string const& title)
{
    auto check = m_db->CreateStatement("SELECT 1 FROM rss_item WHERE feed_id = ? AND guid = ?");
    check->Bind(1, feedId);
    check->Bind(2, guid);

    if (check->Read())
    {
        return false;
    }

    auto insert = m_db->CreateStatement(
        "INSERT INTO rss_item (feed_id, guid, title, added_at) VALUES (?, ?, ?, ?)");
    insert->Bind(1, feedId);
    insert->Bind(2, guid);
    insert->Bind(3, title);
    insert->Bind(4, static_cast<int>(std::time(nullptr)));
    insert->Execute();

    return true;
}

void FeedManager::UnmarkSeen(int feedId, std::string const& guid)
{
    // Called when an item could not be resolved to a torrent, so it is retried
    // on the next pass instead of being skipped forever.
    auto stmt = m_db->CreateStatement("DELETE FROM rss_item WHERE feed_id = ? AND guid = ?");
    stmt->Bind(1, feedId);
    stmt->Bind(2, guid);
    stmt->Execute();
}

std::vector<Feed> FeedManager::GetFeeds()
{
    std::vector<Feed> feeds;

    auto stmt = m_db->CreateStatement(
        "SELECT f.id, f.url, f.name, f.enabled, "
        "       COALESCE(fl.is_regex, 0), COALESCE(fl.include_filter, ''), COALESCE(fl.exclude_filter, '') "
        "FROM rss_feed f "
        "LEFT JOIN rss_feed_filter fl ON fl.feed_id = f.id "
        "ORDER BY f.name");

    while (stmt->Read())
    {
        Feed feed;
        feed.id = stmt->GetInt(0);
        feed.url = stmt->GetString(1);
        feed.name = stmt->GetString(2);
        feed.enabled = stmt->GetInt(3) != 0;
        feed.isRegex = stmt->GetInt(4) != 0;
        feed.includeFilter = stmt->GetString(5);
        feed.excludeFilter = stmt->GetString(6);
        feeds.push_back(feed);
    }

    return feeds;
}

void FeedManager::SaveFeed(Feed& feed)
{
    int const now = static_cast<int>(std::time(nullptr));

    if (feed.id < 0)
    {
        auto stmt = m_db->CreateStatement(
            "INSERT INTO rss_feed (url, name, enabled, created_at) VALUES (?, ?, ?, ?)");
        stmt->Bind(1, feed.url);
        stmt->Bind(2, feed.name);
        stmt->Bind(3, feed.enabled ? 1 : 0);
        stmt->Bind(4, now);
        stmt->Execute();

        auto idStmt = m_db->CreateStatement("SELECT id FROM rss_feed WHERE url = ?");
        idStmt->Bind(1, feed.url);

        if (idStmt->Read())
        {
            feed.id = idStmt->GetInt(0);
        }
    }
    else
    {
        auto stmt = m_db->CreateStatement(
            "UPDATE rss_feed SET url = ?, name = ?, enabled = ?, updated_at = ? WHERE id = ?");
        stmt->Bind(1, feed.url);
        stmt->Bind(2, feed.name);
        stmt->Bind(3, feed.enabled ? 1 : 0);
        stmt->Bind(4, now);
        stmt->Bind(5, feed.id);
        stmt->Execute();
    }

    auto del = m_db->CreateStatement("DELETE FROM rss_feed_filter WHERE feed_id = ?");
    del->Bind(1, feed.id);
    del->Execute();

    if (!feed.includeFilter.empty() || !feed.excludeFilter.empty())
    {
        auto ins = m_db->CreateStatement(
            "INSERT INTO rss_feed_filter (feed_id, is_regex, include_filter, exclude_filter) VALUES (?, ?, ?, ?)");
        ins->Bind(1, feed.id);
        ins->Bind(2, feed.isRegex ? 1 : 0);
        ins->Bind(3, feed.includeFilter);
        ins->Bind(4, feed.excludeFilter);
        ins->Execute();
    }

    // Fetch immediately so the user gets feedback right after saving.
    RefreshFeed(feed);
}

void FeedManager::RemoveFeed(int id)
{
    auto items = m_db->CreateStatement("DELETE FROM rss_item WHERE feed_id = ?");
    items->Bind(1, id);
    items->Execute();

    auto stmt = m_db->CreateStatement("DELETE FROM rss_feed WHERE id = ?");
    stmt->Bind(1, id);
    stmt->Execute();
}
