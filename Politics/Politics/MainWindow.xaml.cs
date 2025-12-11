using CoronaInterface;
using CoronaLib;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics.Contracts;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Security.AccessControl;
using System.Threading;
using Windows.Foundation;
using Windows.Foundation.Collections;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace Politics
{
    /// <summary>
    /// An empty window that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainWindow : Window
    {
        private CoronaDatabase coronaDatabase = new CoronaDatabase();
        private CoronaStatusModel coronaStatusModel = new CoronaStatusModel();

        public CoronaDatabase CoronaDatabase { get => coronaDatabase; }
        public CoronaStatusModel CoronaStatusModel { get => coronaStatusModel; }

        /// <summary>
        /// Invoked when the application is launched.
        /// </summary>
        /// <param name="args">Details about the launch request and process.</param>
        /// 

        public string DatabasePath => Windows.Storage.ApplicationData.Current.LocalFolder.Path;
        public string ConfigPath => System.IO.Path.Combine(System.IO.Path.GetDirectoryName(Environment.ProcessPath), "configuration");

        public MainWindow()
        {
            InitializeComponent();
            CoronaSystem cm = new CoronaSystem();
            cm.SystemMonitoring = coronaStatusModel;
            DispatcherQueue.EnsureSystemDispatcherQueue();
            MessageQueue = DispatcherQueue.GetForCurrentThread();
            MainNavigationView.ItemInvoked += NavView_ItemInvoked;
            MainNavigationView.SelectionChanged += NavView_SelectionChanged;
        }

        private void NavView_SelectionChanged(NavigationView sender, NavigationViewSelectionChangedEventArgs args)
        {
        }

        private void NavView_ItemInvoked(NavigationView sender, NavigationViewItemInvokedEventArgs args)
        {
            if (args.InvokedItemContainer?.Tag is string pageTag)
            {
                var pageType = Type.GetType(pageTag);
                if (pageType != null && ContentFrame.CurrentSourcePageType != pageType)
                    ContentFrame.Navigate(pageType);
            }
        }

        public DispatcherQueue MessageQueue { get; private set; }

        public static void CreateDatabase(MainWindow app)
        {
            
            DatabaseConfiguration? config = new DatabaseConfiguration();

            string config_file_name = System.IO.Path.Combine(app.ConfigPath, "config.json");
            string config_content = File.ReadAllText(config_file_name);

            config = JsonConvert.DeserializeObject<DatabaseConfiguration>(config_content);

            if (config != null)
            {
                
                config.DatabasePath = app.DatabasePath;
                config.SchemaPath = app.ConfigPath;

                if (app.coronaDatabase != null)
                {
                    app.coronaDatabase.Dispose();
                }

                app.coronaDatabase = new CoronaDatabase();
                app.coronaDatabase.CreateDatabase(config);
            }
        }

        private void Grid_Loaded(object sender, RoutedEventArgs e)
        {
            ContentFrame.Navigate(typeof(CoronaStatusPage));
        }
    }
}
