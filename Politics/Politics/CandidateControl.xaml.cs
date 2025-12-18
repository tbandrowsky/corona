using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Dynamic;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace Politics;

public partial class CandidateControl : UserControl
{


    ExpandoObject candidate;

    public CandidateControl()
    {
        InitializeComponent();
    }


    public ExpandoObject Candidate
    {
        get => (dynamic)GetValue(CandidateProperty);
        set => SetValue(CandidateProperty, value);
    }

    DependencyProperty CandidateProperty = DependencyProperty.Register(
        "Candidate",
        typeof(ExpandoObject),
        typeof(CandidateControl),
        new PropertyMetadata(null, OnCoronaDataChanged)
    );

    private static void OnCoronaDataChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is CandidateControl card && e.NewValue is ExpandoObject newData)
        {
            card.DataContext = newData;
        }
    }
}
