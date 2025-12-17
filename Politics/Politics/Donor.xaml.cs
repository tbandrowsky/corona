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
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace Politics;

public partial class Donor : UserControl, INotifyPropertyChanged
{
    public Donor()
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
            full_name = current?["full_name"] as string ?? string.Empty;
            city = current?["city"] as string ?? string.Empty;
            state = current?["state"] as string ?? string.Empty;
            zip = current?["zip"] as string ?? string.Empty;    
            employer = current?["employer"] as string ?? string.Empty;
            occupation = current?["occupation"] as string ?? string.Empty;  

            OnPropertyChanged("CObject");
            OnPropertyChanged("full_name");
            OnPropertyChanged("city");
            OnPropertyChanged("state");
            OnPropertyChanged("zip");
            OnPropertyChanged("employer");
            OnPropertyChanged("occupation");
        }
    }

    public string full_name { get; set; } = string.Empty;
    public string city { get; set; } = string.Empty;
    public string state { get; set; } = string.Empty;
    public string zip { get; set; } = string.Empty;
    public string employer { get; set; } = string.Empty;
    public string occupation { get; set; } = string.Empty;

    public event PropertyChangedEventHandler PropertyChanged;

    protected virtual void OnPropertyChanged(string propertyName)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }

}
