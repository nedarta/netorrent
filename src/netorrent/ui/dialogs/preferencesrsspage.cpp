#include "preferencesrsspage.hpp"

#include <wx/listctrl.h>
#include <wx/statline.h>

#include "../../rss/feedmanager.hpp"
#include "../../rss/rssfeed.hpp"
#include "../translator.hpp"

using pt::UI::Dialogs::PreferencesRssPage;

PreferencesRssPage::PreferencesRssPage(wxWindow* parent, std::shared_ptr<pt::Core::Configuration> cfg, pt::RSS::FeedManager* feedManager)
    : wxPanel(parent),
    m_cfg(cfg),
    m_feedManager(feedManager)
{
    auto desc = new wxStaticText(this, wxID_ANY, i18n("rss_description"));
    desc->Wrap(FromDIP(460));

    m_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    m_list->AppendColumn(i18n("feed_name"), wxLIST_FORMAT_LEFT, FromDIP(140));
    m_list->AppendColumn(i18n("feed_url"), wxLIST_FORMAT_LEFT, FromDIP(240));
    m_list->AppendColumn(i18n("feed_enabled"), wxLIST_FORMAT_LEFT, FromDIP(70));

    m_name = new wxTextCtrl(this, wxID_ANY);
    m_url = new wxTextCtrl(this, wxID_ANY);
    m_include = new wxTextCtrl(this, wxID_ANY);
    m_exclude = new wxTextCtrl(this, wxID_ANY);
    m_regex = new wxCheckBox(this, wxID_ANY, i18n("feed_use_regex"));
    m_enabled = new wxCheckBox(this, wxID_ANY, i18n("feed_enabled"));
    m_enabled->SetValue(true);

    auto form = new wxFlexGridSizer(2, FromDIP(6), FromDIP(6));
    form->AddGrowableCol(1, 1);
    form->Add(new wxStaticText(this, wxID_ANY, i18n("feed_name")), 0, wxALIGN_CENTER_VERTICAL);
    form->Add(m_name, 1, wxEXPAND);
    form->Add(new wxStaticText(this, wxID_ANY, i18n("feed_url")), 0, wxALIGN_CENTER_VERTICAL);
    form->Add(m_url, 1, wxEXPAND);
    form->Add(new wxStaticText(this, wxID_ANY, i18n("feed_include_filter")), 0, wxALIGN_CENTER_VERTICAL);
    form->Add(m_include, 1, wxEXPAND);
    form->Add(new wxStaticText(this, wxID_ANY, i18n("feed_exclude_filter")), 0, wxALIGN_CENTER_VERTICAL);
    form->Add(m_exclude, 1, wxEXPAND);
    form->AddSpacer(0);
    form->Add(m_regex, 0);
    form->AddSpacer(0);
    form->Add(m_enabled, 0);

    m_addOrUpdate = new wxButton(this, wxID_ANY, i18n("add_feed"));
    m_remove = new wxButton(this, wxID_ANY, i18n("remove"));
    auto refresh = new wxButton(this, wxID_ANY, i18n("refresh_feeds"));

    auto buttons = new wxBoxSizer(wxHORIZONTAL);
    buttons->Add(m_addOrUpdate, 0, wxRIGHT, FromDIP(6));
    buttons->Add(m_remove, 0, wxRIGHT, FromDIP(6));
    buttons->AddStretchSpacer();
    buttons->Add(refresh, 0);

    auto sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(desc, 0, wxEXPAND | wxBOTTOM, FromDIP(8));
    sizer->Add(m_list, 1, wxEXPAND | wxBOTTOM, FromDIP(8));
    sizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxBOTTOM, FromDIP(8));
    sizer->Add(form, 0, wxEXPAND | wxBOTTOM, FromDIP(8));
    sizer->Add(buttons, 0, wxEXPAND);

    this->SetSizer(sizer);

    m_addOrUpdate->Bind(wxEVT_BUTTON, &PreferencesRssPage::OnAddOrUpdate, this);
    m_remove->Bind(wxEVT_BUTTON, &PreferencesRssPage::OnRemove, this);
    refresh->Bind(wxEVT_BUTTON, &PreferencesRssPage::OnRefresh, this);
    m_list->Bind(wxEVT_LIST_ITEM_SELECTED, &PreferencesRssPage::OnSelectionChanged, this);

    ReloadList();
}

PreferencesRssPage::~PreferencesRssPage()
{
}

bool PreferencesRssPage::IsValid()
{
    return true;
}

void PreferencesRssPage::Save()
{
    // Feeds are persisted as they are added/edited/removed.
}

int PreferencesRssPage::GetSelectedFeedId()
{
    long item = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

    if (item < 0)
    {
        return -1;
    }

    return static_cast<int>(m_list->GetItemData(item));
}

void PreferencesRssPage::ReloadList()
{
    m_list->DeleteAllItems();

    for (auto const& feed : m_feedManager->GetFeeds())
    {
        long row = m_list->InsertItem(m_list->GetItemCount(), wxString::FromUTF8(feed.name));
        m_list->SetItem(row, 1, wxString::FromUTF8(feed.url));
        m_list->SetItem(row, 2, feed.enabled ? i18n("yes") : i18n("no"));
        m_list->SetItemData(row, feed.id);
    }
}

void PreferencesRssPage::ClearForm()
{
    m_name->ChangeValue(wxEmptyString);
    m_url->ChangeValue(wxEmptyString);
    m_include->ChangeValue(wxEmptyString);
    m_exclude->ChangeValue(wxEmptyString);
    m_regex->SetValue(false);
    m_enabled->SetValue(true);
    m_addOrUpdate->SetLabel(i18n("add_feed"));
}

void PreferencesRssPage::OnSelectionChanged(wxListEvent& evt)
{
    int id = static_cast<int>(m_list->GetItemData(evt.GetIndex()));

    for (auto const& feed : m_feedManager->GetFeeds())
    {
        if (feed.id != id)
        {
            continue;
        }

        m_name->ChangeValue(wxString::FromUTF8(feed.name));
        m_url->ChangeValue(wxString::FromUTF8(feed.url));
        m_include->ChangeValue(wxString::FromUTF8(feed.includeFilter));
        m_exclude->ChangeValue(wxString::FromUTF8(feed.excludeFilter));
        m_regex->SetValue(feed.isRegex);
        m_enabled->SetValue(feed.enabled);
        m_addOrUpdate->SetLabel(i18n("update_feed"));
        break;
    }
}

void PreferencesRssPage::OnAddOrUpdate(wxCommandEvent&)
{
    std::string url = m_url->GetValue().ToStdString();

    if (url.empty())
    {
        wxMessageBox(i18n("feed_url_required"), i18n("rss"), wxICON_WARNING | wxOK, this);
        return;
    }

    pt::RSS::Feed feed;
    feed.id = GetSelectedFeedId();
    feed.url = url;
    feed.name = m_name->GetValue().ToStdString();
    feed.includeFilter = m_include->GetValue().ToStdString();
    feed.excludeFilter = m_exclude->GetValue().ToStdString();
    feed.isRegex = m_regex->GetValue();
    feed.enabled = m_enabled->GetValue();

    if (feed.name.empty())
    {
        feed.name = url;
    }

    m_feedManager->SaveFeed(feed);

    ClearForm();
    ReloadList();
}

void PreferencesRssPage::OnRemove(wxCommandEvent&)
{
    int id = GetSelectedFeedId();

    if (id < 0)
    {
        return;
    }

    m_feedManager->RemoveFeed(id);

    ClearForm();
    ReloadList();
}

void PreferencesRssPage::OnRefresh(wxCommandEvent&)
{
    m_feedManager->RefreshAll();
}
