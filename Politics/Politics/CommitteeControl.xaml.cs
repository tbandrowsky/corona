using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.Dynamic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace Politics
{
    public sealed partial class CommitteeControl : UserControl
    {
        ExpandoObject candidate;

        public CommitteeControl()
        {
            InitializeComponent();
        }


        public ExpandoObject Committee
        {
            get => (dynamic)GetValue(CommitteeProperty);
            set => SetValue(CommitteeProperty, value);
        }

        DependencyProperty CommitteeProperty = DependencyProperty.Register(
            "Committee",
            typeof(ExpandoObject),
            typeof(CommitteeControl),
            new PropertyMetadata(null, OnCoronaDataChanged)
        );

        private static void OnCoronaDataChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is CommitteeControl card && e.NewValue is ExpandoObject newData)
            {
                card.DataContext = newData;
            }
        }
    }
}
