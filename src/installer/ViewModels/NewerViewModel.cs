using System;
using System.Windows.Input;

namespace NeTorrentBootstrapper.ViewModels
{
    public sealed class NewerViewModel : PropertyNotifyBase
    {
        private readonly MainViewModel _mainModel;

        public NewerViewModel(MainViewModel mainModel)
        {
            _mainModel = mainModel ?? throw new ArgumentNullException(nameof(mainModel));
        }

        public ICommand CloseCommand => _mainModel.CloseCommand;
    }
}
