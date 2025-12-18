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

namespace Politics
{
    public partial class Card : UserControl

    {

        public ExpandoObject card { get; set; }

        public Card()
        {
            InitializeComponent();
        }

        public ExpandoObject CObject
        {
            get => (dynamic)GetValue(CObjectProperty);
            set => SetValue(CObjectProperty, value);
        }

        DependencyProperty CObjectProperty = DependencyProperty.Register(
            "CObject",
            typeof(ExpandoObject),
            typeof(Card),
            new PropertyMetadata(null, OnCoronaDataChanged)
        );

        private static void OnCoronaDataChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is Card card && e.NewValue is ExpandoObject newData)
            {
                card.DataContext = newData;
            }
        }
    }
}
