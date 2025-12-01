using CoronaLib;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace Politics
{
    public sealed partial class CoronaSchemaLoad : UserControl
    {
        public CoronaStatusModel CoronaStatusModel
        {
            get => (CoronaStatusModel)GetValue(CoronaStatusModelProperty);
            set => SetValue(CoronaStatusModelProperty, value);
        }

        public static readonly DependencyProperty CoronaStatusModelProperty =
            DependencyProperty.Register(
                nameof(CoronaStatusModel),
                typeof(CoronaStatusModel),
                typeof(CoronaSchemaLoad),
                new PropertyMetadata(null, OnCoronaStatusModelChanged));

        private static void OnCoronaStatusModelChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = (CoronaSchemaLoad)d;
            // Handle property changes here if needed.
            // Example: update internal bindings or subscribe/unsubscribe events from the model.
            // control.DataContext = e.NewValue; // optional: enable if the control should use the model as its DataContext
        }
        public CoronaSchemaLoad()
        {
            InitializeComponent();
            DataContext = CoronaStatusModel;
        }
    }
}
