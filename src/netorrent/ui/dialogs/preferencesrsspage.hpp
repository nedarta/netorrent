#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <memory>

namespace pt::Core { class Configuration; }
namespace pt::RSS { class FeedManager; }

class wxListCtrl;
class wxListEvent;

namespace pt::UI::Dialogs
{
    class PreferencesRssPage : public wxPanel
    {
    public:
        PreferencesRssPage(wxWindow* parent, std::shared_ptr<Core::Configuration> cfg, RSS::FeedManager* feedManager);
        virtual ~PreferencesRssPage();

        bool IsValid();
        void Save();

    private:
        void OnAddOrUpdate(wxCommandEvent&);
        void OnRemove(wxCommandEvent&);
        void OnRefresh(wxCommandEvent&);
        void OnSelectionChanged(wxListEvent&);
        void OnFeedUpdated(wxCommandEvent&);
        void ReloadList();
        void ClearForm();
        int GetSelectedFeedId();

        std::shared_ptr<Core::Configuration> m_cfg;
        RSS::FeedManager* m_feedManager;

        wxListCtrl* m_list;
        wxTextCtrl* m_name;
        wxTextCtrl* m_url;
        wxTextCtrl* m_include;
        wxTextCtrl* m_exclude;
        wxCheckBox* m_regex;
        wxCheckBox* m_enabled;
        wxButton* m_addOrUpdate;
        wxButton* m_remove;
        wxStaticText* m_status;
    };
}
