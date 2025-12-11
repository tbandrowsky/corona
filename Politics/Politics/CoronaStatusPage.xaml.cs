using CommunityToolkit.HighPerformance.Buffers;
using CoronaCharts;
using Microsoft.Graphics.Canvas.Brushes;
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
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using System.Runtime.CompilerServices;
using Windows.UI;
using Microsoft.UI;
using Windows.UI.ApplicationSettings;
using Windows.ApplicationModel.VoiceCommands; // Color

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace Politics
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class CoronaStatusPage : Page, INotifyPropertyChanged
    {
        public CoronaStatusPage()
        {
            this.InitializeComponent();
            this.Loaded += Page_Loaded;
        }

        // INotifyPropertyChanged
        public event PropertyChangedEventHandler? PropertyChanged;
        private void RaisePropertyChanged([CallerMemberName] string? name = null) =>
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));

        // Color properties
        private Color _userCommandColor = Colors.LightGray;
        public Color UserCommandColor { get => _userCommandColor; set { if (_userCommandColor.Equals(value) == false) { _userCommandColor = value; RaisePropertyChanged(); } } }

        private Color _commandColor = Colors.LightGray;
        public Color CommandColor { get => _commandColor; set { if (_commandColor.Equals(value) == false) { _commandColor = value; RaisePropertyChanged(); } } }

        private Color _jobColor = Colors.LightGray;
        public Color JobColor { get => _jobColor; set { if (_jobColor.Equals(value) == false) { _jobColor = value; RaisePropertyChanged(); } } }

        private Color _jobSectionColor = Colors.LightGray;
        public Color JobSectionColor { get => _jobSectionColor; set { if (_jobSectionColor.Equals(value) == false) { _jobSectionColor = value; RaisePropertyChanged(); } } }

        private Color _functionColor = Colors.LightGray;
        public Color FunctionColor { get => _functionColor; set { if (_functionColor.Equals(value) == false) { _functionColor = value; RaisePropertyChanged(); } } }

        private Color _informationColor = Colors.LightGray;
        public Color InformationColor { get => _informationColor; set { if (_informationColor.Equals(value) == false) { _informationColor = value; RaisePropertyChanged(); } } }

        private Color _warningColor = Colors.Yellow;
        public Color WarningColor { get => _warningColor; set { if (_warningColor.Equals(value) == false) { _warningColor = value; RaisePropertyChanged(); } } }

        private Color _exceptionColor = Colors.Red;
        public Color ExceptionColor { get => _exceptionColor; set { if (_exceptionColor.Equals(value) == false) { _exceptionColor = value; RaisePropertyChanged(); } } }

        // Text properties
        private string _userCommandApi = "";
        public string UserCommandApi { get => _userCommandApi; set { if (_userCommandApi != value) { _userCommandApi = value; RaisePropertyChanged(); } } }
        private string _userCommandTopic = "";
        public string UserCommandTopic { get => _userCommandTopic; set { if (_userCommandTopic != value) { _userCommandTopic = value; RaisePropertyChanged(); } } }
        private string _userCommandMessage = "";
        public string UserCommandMessage { get => _userCommandMessage; set { if (_userCommandMessage != value) { _userCommandMessage = value; RaisePropertyChanged(); } } }
        private string _userCommandStart = "";
        public string UserCommandStart { get => _userCommandStart; set { if (_userCommandStart != value) { _userCommandStart = value; RaisePropertyChanged(); } } }
        private string _userCommandElapsed = "";
        public string UserCommandElapsed { get => _userCommandElapsed; set { if (_userCommandElapsed != value) { _userCommandElapsed = value; RaisePropertyChanged(); } } }

        private string _commandApi = "";
        public string CommandApi { get => _commandApi; set { if (_commandApi != value) { _commandApi = value; RaisePropertyChanged(); } } }
        private string _commandTopic = "";
        public string CommandTopic { get => _commandTopic; set { if (_commandTopic != value) { _commandTopic = value; RaisePropertyChanged(); } } }
        private string _commandMessage = "";
        public string CommandMessage { get => _commandMessage; set { if (_commandMessage != value) { _commandMessage = value; RaisePropertyChanged(); } } }
        private string _commandStart = "";
        public string CommandStart { get => _commandStart; set { if (_commandStart != value) { _commandStart = value; RaisePropertyChanged(); } } }
        private string _commandElapsed = "";
        public string CommandElapsed { get => _commandElapsed; set { if (_commandElapsed != value) { _commandElapsed = value; RaisePropertyChanged(); } } }


        private string _jobApi = "";
        public string JobApi { get => _jobApi; set { if (_jobApi != value) { _jobApi = value; RaisePropertyChanged(); } } }
        private string _jobTopic = "";
        public string JobTopic { get => _jobTopic; set { if (_jobTopic != value) { _jobTopic = value; RaisePropertyChanged(); } } }
        private string _jobMessage = "";
        public string JobMessage { get => _jobMessage; set { if (_jobMessage != value) { _jobMessage = value; RaisePropertyChanged(); } } }
        private string _jobStart = "";
        public string JobStart { get => _jobStart; set { if (_jobStart != value) { _jobStart = value; RaisePropertyChanged(); } } }
        private string _jobElapsed = "";
        public string JobElapsed { get => _jobElapsed; set { if (_jobElapsed != value) { _jobElapsed = value; RaisePropertyChanged(); } } }

        private string _jobSectionApi = "";
        public string JobSectionApi { get => _jobSectionApi; set { if (_jobSectionApi != value) { _jobSectionApi = value; RaisePropertyChanged(); } } }
        private string _jobSectionTopic = "";
        public string JobSectionTopic { get => _jobSectionTopic; set { if (_jobSectionTopic != value) { _jobSectionTopic = value; RaisePropertyChanged(); } } }
        private string _jobSectionMessage = "";
        public string JobSectionMessage { get => _jobSectionMessage; set { if (_jobSectionMessage != value) { _jobSectionMessage = value; RaisePropertyChanged(); } } }
        private string _jobSectionStart = "";
        public string JobSectionStart { get => _jobSectionStart; set { if (_jobSectionStart != value) { _jobSectionStart = value; RaisePropertyChanged(); } } }
        private string _jobSectionElapsed = "";
        public string JobSectionElapsed { get => _jobSectionElapsed; set { if (_jobSectionElapsed != value) { _jobSectionElapsed = value; RaisePropertyChanged(); } } }

        private string _functionApi = "";
        public string FunctionApi { get => _functionApi; set { if (_functionApi != value) { _functionApi = value; RaisePropertyChanged(); } } }
        private string _functionTopic = "";
        public string FunctionTopic { get => _functionTopic; set { if (_functionTopic != value) { _functionTopic = value; RaisePropertyChanged(); } } }
        private string _functionMessage = "";
        public string FunctionMessage { get => _functionMessage; set { if (_functionMessage != value) { _functionMessage = value; RaisePropertyChanged(); } } }
        private string _functionStart = "";
        public string FunctionStart { get => _functionStart; set { if (_functionStart != value) { _functionStart = value; RaisePropertyChanged(); } } }
        private string _functionElapsed = "";
        public string FunctionElapsed { get => _functionElapsed; set { if (_functionElapsed != value) { _functionElapsed = value; RaisePropertyChanged(); } } }

        private string _informationApi = "";
        public string InformationApi { get => _informationApi; set { if (_informationApi != value) { _informationApi = value; RaisePropertyChanged(); } } }
        private string _informationTopic = "";
        public string InformationTopic { get => _informationTopic; set { if (_informationTopic != value) { _informationTopic = value; RaisePropertyChanged(); } } }
        private string _informationMessage = "";
        public string InformationMessage { get => _informationMessage; set { if (_informationMessage != value) { _informationMessage = value; RaisePropertyChanged(); } } }
        private string _informationStart = "";
        public string InformationStart { get => _informationStart; set { if (_informationStart != value) { _informationStart = value; RaisePropertyChanged(); } } }
        private string _warningText = "";

        private string _warningApi = "";
        public string WarningApi { get => _warningApi; set { if (_warningApi != value) { _warningApi = value; RaisePropertyChanged(); } } }
        private string _warningTopic = "";
        public string WarningTopic { get => _warningTopic; set { if (_warningTopic != value) { _warningTopic = value; RaisePropertyChanged(); } } }
        private string _warningMessage = "";
        public string WarningMessage { get => _warningMessage; set { if (_warningMessage != value) { _warningMessage = value; RaisePropertyChanged(); } } }
        public string WarningText { get => _warningText; set { if (_warningText != value) { _warningText = value; RaisePropertyChanged(); } } }
        private string _warningStart = "";
        public string WarningStart { get => _warningStart; set { if (_warningStart != value) { _warningStart = value; RaisePropertyChanged(); } } }


        private string _exceptionApi = "";
        public string ExceptionApi { get => _exceptionApi; set { if (_exceptionApi != value) { _exceptionApi = value; RaisePropertyChanged(); } } }
        private string _exceptionTopic = "";
        public string ExceptionTopic { get => _exceptionTopic; set { if (_exceptionTopic != value) { _exceptionTopic = value; RaisePropertyChanged(); } } }
        private string _exceptionMessage = "";
        public string ExceptionMessage { get => _exceptionMessage; set { if (_exceptionMessage != value) { _exceptionMessage = value; RaisePropertyChanged(); } } }
        private string _exceptionStart = "";
        public string ExceptionStart { get => _exceptionStart; set { if (_exceptionStart != value) { _exceptionStart = value; RaisePropertyChanged(); } } }

        TimeChartSeriesCollection historyChartModel;
        SummaryChartSeriesCollection performanceChartModel;
        SummaryChartSeriesCollection distributionChartModel;

        private void Page_Loaded(object sender, RoutedEventArgs e)
        {
            DataContext = this;
            App.CurrentApp.CoronaStatusModel.MessageReceived += OnMessageReceived;
            legendControl.LegendItemChecked += LegendControl_LegendItemChecked;
            Refresh(true);
            foreach (var msg in App.CurrentApp.CoronaStatusModel.Messages)
            {
                UpdateStatusPanel(msg);
            }
        }

        private void LegendControl_LegendItemChecked(object? sender, LegendItem e)
        {
            GlobalPalette.Current.Select(e);
            Refresh(false);
        }

        public void Refresh(bool update_legend)
        {

            historyChartModel = new TimeChartSeriesCollection();
            historyChartModel.Series = App.CurrentApp.CoronaStatusModel.Messages
                .Where(a => GlobalPalette.Current.IsSelected(a))
                .GroupBy(a => a.Api + "\n" + a.Topic)
                .Select(g => new TimeChartSeries
                {
                    Name = g.Key,
                    Points = g.Select(m => new TimePoint
                    {
                        Time = m.StartTime,
                        Value = m.ElapsedSeconds
                    }).ToList()
                }).ToList();

            performanceChartModel = new SummaryChartSeriesCollection();
            performanceChartModel.Series = App.CurrentApp.CoronaStatusModel.Messages
                .Where( a => a.Api == "Function" && a.ElapsedSeconds > 0)
                .Where(a => GlobalPalette.Current.IsSelected(a))
                .GroupBy(a => new { a.Topic, Message = a.Message.FirstWord() })
                .Select(g => new SummaryChartSeries
                {
                    Name = g.Key.Topic,
                    Value = g.Average(m => m.ElapsedSeconds),
                    FillColor = GlobalPalette.Current.GetApiColor("Function", g.Key.Topic, g.Key.Message),
                    BorderColor = GlobalPalette.Current.GetApiColor("Function", g.Key.Topic, g.Key.Message)
                }).ToList();

            distributionChartModel = new SummaryChartSeriesCollection();
            distributionChartModel.Series = App.CurrentApp.CoronaStatusModel.Messages
                .Where(a => a.Api == "Function" && a.ElapsedSeconds > 0)
                .Where(a => GlobalPalette.Current.IsSelected(a))
                .GroupBy(a => new { a.Topic, Message = a.Message.FirstWord() })
                .Select(g => new SummaryChartSeries
                {
                    Name = g.Key.Topic,
                    Value = g.Sum(m => m.ElapsedSeconds),
                    FillColor = GlobalPalette.Current.GetApiColor("Function", g.Key.Topic, g.Key.Message),
                    BorderColor = GlobalPalette.Current.GetApiColor("Function", g.Key.Topic, g.Key.Message)
                }).ToList();

            performanceChart.Series = performanceChartModel;
            distributionChart.Series = distributionChartModel;
            activitygrid.Refresh();

            if (update_legend)
            {
                var legendItems = GlobalPalette.Current.GetLegend();
                foreach (var li in legendItems)
                {
                    var existingli = legendControl.Find(li.Api, li.Topic);
                    if (existingli != null)
                    {
                        li.IsSelected = existingli.IsSelected;
                    }
                }

                legendControl.Items.Clear();
                foreach (var item in legendItems)
                {
                    legendControl.Items.Add(item);
                }
            }
        }


        private void UpdateStatusPanel(CoronaMessage message)
        {
            string api = message.Api;

            if (api == "Activity")
            {
                api = "Information";
            }

            switch (api)
            {
                case "UserCommand":
                    UserCommandColor = GlobalPalette.Current.GetApiColor(message.Api, message.Topic, message.Message.FirstWord()).ToColor();
                    UserCommandApi = message.Api;
                    UserCommandTopic = message.Topic;
                    UserCommandMessage = message.Message;
                    UserCommandStart = message.StartTimeString;
                    UserCommandElapsed = message.ElapsedMillisecondsString;
                    goto SetCommand;
                case "Command":
                    CommandColor = GlobalPalette.Current.GetApiColor(message.Api, message.Topic, message.Message.FirstWord()).ToColor();
                    CommandApi = message.Api;
                    CommandTopic = message.Topic;
                    CommandMessage = message.Message;
                    CommandStart = message.StartTimeString;
                    CommandElapsed = message.ElapsedMillisecondsString;
                    CommandStatus.Visibility = Visibility.Visible;
                    goto SetJob;
                case "Job":
                    JobColor = GlobalPalette.Current.GetApiColor(message.Api, message.Topic, message.Message.FirstWord()).ToColor();
                    JobApi = message.Api;
                    JobTopic = message.Topic;
                    JobMessage = message.Message;
                    JobStart = message.StartTimeString;
                    JobElapsed = message.ElapsedMillisecondsString;
                    JobStatus.Visibility = Visibility.Visible;
                    goto SetJobSection;
                case "JobSection":
                    JobSectionColor = GlobalPalette.Current.GetApiColor(message.Api, message.Topic, message.Message.FirstWord()).ToColor();
                    JobSectionApi = message.Api;
                    JobSectionTopic = message.Topic;
                    JobSectionMessage = message.Message;
                    JobSectionStart = message.StartTimeString;
                    JobSectionElapsed = message.ElapsedMillisecondsString;
                    JobStatus.Visibility = Visibility.Visible;
                    JobSectionStatus.Visibility = Visibility.Visible;
                    goto SetFunction;
                case "Function":
                    FunctionColor = GlobalPalette.Current.GetApiColor(message.Api, message.Topic, message.Message.FirstWord()).ToColor();
                    FunctionApi = message.Api;
                    FunctionTopic = message.Topic;
                    FunctionMessage = message.Message;
                    FunctionStart = message.StartTimeString;
                    FunctionElapsed = message.ElapsedMillisecondsString;
                    JobStatus.Visibility = Visibility.Visible;
                    JobSectionStatus.Visibility = Visibility.Visible;
                    FunctionStatus.Visibility = Visibility.Visible;
                    goto SetInformation;
                case "Information":
                    InformationColor = GlobalPalette.Current.GetApiColor(message.Api, message.Topic, message.Message.FirstWord()).ToColor();
                    InformationApi = message.Api;
                    InformationTopic = message.Topic;
                    InformationMessage = message.Message;
                    InformationStart = message.StartTimeString;
                    JobStatus.Visibility = Visibility.Visible;
                    JobSectionStatus.Visibility = Visibility.Visible;
                    FunctionStatus.Visibility = Visibility.Visible;
                    InformationStatus.Visibility = Visibility.Visible;
                    goto SetWarning;
                case "Warning":
                    WarningColor = GlobalPalette.Current.GetApiColor(message.Api, message.Topic, message.Message.FirstWord()).ToColor();
                    WarningApi = message.Api;
                    WarningTopic = message.Topic;
                    WarningMessage = message.Message;
                    WarningStart = message.StartTimeString;
                    JobStatus.Visibility = Visibility.Visible;
                    JobSectionStatus.Visibility = Visibility.Visible;
                    FunctionStatus.Visibility = Visibility.Visible;
                    InformationStatus.Visibility = Visibility.Visible;
                    WarningStatus.Visibility = Visibility.Visible;
                    goto SetException;
                case "Exception":
                    ExceptionColor = GlobalPalette.Current.GetApiColor(message.Api, message.Topic, message.Message.FirstWord()).ToColor();
                    ExceptionApi = message.Api;
                    ExceptionTopic = message.Topic;
                    ExceptionMessage = message.Message;
                    ExceptionStart = message.StartTimeString;
                    JobStatus.Visibility = Visibility.Visible;
                    JobSectionStatus.Visibility = Visibility.Visible;
                    FunctionStatus.Visibility = Visibility.Visible;
                    InformationStatus.Visibility = Visibility.Visible;
                    WarningStatus.Visibility = Visibility.Visible;
                    ExceptionStatus.Visibility = Visibility.Visible;
                    return;
            }


        SetCommand:
            CommandStatus.Visibility = Visibility.Collapsed;
        SetJob:
            JobStatus.Visibility = Visibility.Collapsed;
        SetJobSection:
            JobSectionStatus.Visibility = Visibility.Collapsed;
            InformationStatus.Visibility = Visibility.Collapsed;
        SetFunction:
            FunctionStatus.Visibility = Visibility.Collapsed;
        SetInformation:
        SetWarning:
            WarningStatus.Visibility = Visibility.Collapsed;
        SetException:
            ExceptionStatus.Visibility = Visibility.Collapsed;
        }


        DateTime lastRefreshTime = DateTime.MinValue;

        private void OnMessageReceived(CoronaMessage message)
        {
            DispatcherQueue.TryEnqueue(() =>
                {
                    if ((DateTime.Now - lastRefreshTime).TotalMilliseconds > 500)
                    {
                        lastRefreshTime = DateTime.Now;
                        Refresh(true);
                    }
                    UpdateStatusPanel(message);
                }
            );
        }

    }
}
