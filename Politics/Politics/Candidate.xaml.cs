using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using Politics.models;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace Politics;

public partial class Candidate : UserControl, INotifyPropertyChanged
{


    public Candidate()
    {
        InitializeComponent();
        DataContext = this;
    }

    private cobject current;

    public cobject CObject
    {
        get => current;
        set
        {
            current = value;
            OnPropertyChanged("CObject");
            full_name = current?["full_name"] as string ?? string.Empty;
            candidate_id = current?["candidate_id"] as string ?? string.Empty;
            political_party = current?["political_party"] as string ?? string.Empty;
            election_year = current?["election_year"] as string ?? string.Empty;
            street1 = current?["street1"] as string ?? string.Empty;
            street2 = current?["street2"] as string ?? string.Empty;
            city = current?["city"] as string ?? string.Empty;
            state = current?["state"] as string ?? string.Empty;
            zip = current?["zip"] as string ?? string.Empty;
            OnPropertyChanged("full_name");
            OnPropertyChanged("candidate_id");
            OnPropertyChanged("political_party");
            OnPropertyChanged("election_year");
            OnPropertyChanged("street1");
            OnPropertyChanged("street2");
            OnPropertyChanged("city");
            OnPropertyChanged("state");
            OnPropertyChanged("zip");
        }
    }

    public string full_name { get; set; } = string.Empty;
    public string candidate_id { get; set; } = string.Empty;
    public string political_party { get; set; } = string.Empty;
    public string election_year { get; set; } = string.Empty;
    public string street1 { get; set; } = string.Empty;
    public string street2 { get; set; } = string.Empty;
    public string city { get; set; } = string.Empty;
    public string state { get; set; } = string.Empty;
    public string zip { get; set; }  = string.Empty;

    public event PropertyChangedEventHandler PropertyChanged;

    protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}
