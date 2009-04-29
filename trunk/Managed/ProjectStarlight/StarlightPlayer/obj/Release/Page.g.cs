#pragma checksum "C:\Documents and Settings\mpoindexter\My Documents\Visual Studio 2008\Projects\StarlightPlayer\StarlightPlayer\Page.xaml" "{406ea660-64cf-4c82-b6f0-42d48172a799}" "EC15D7A48AE7B345E77AAD46B26CF7CB"
//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version:2.0.50727.3053
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

using System;
using System.Windows;
using System.Windows.Automation;
using System.Windows.Automation.Peers;
using System.Windows.Automation.Provider;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using System.Windows.Resources;
using System.Windows.Shapes;
using System.Windows.Threading;


namespace Starlight.SamplePlayer {
    
    
    public partial class Page : System.Windows.Controls.UserControl {
        
        internal System.Windows.Controls.Canvas ParentCanvas;
        
        internal System.Windows.Controls.Grid PlayerGrid;
        
        internal System.Windows.Controls.TextBlock StatusText;
        
        internal System.Windows.Controls.TextBlock BufferLevelText;
        
        internal System.Windows.Controls.TextBlock PositionText;
        
        internal System.Windows.Controls.MediaElement MediaPlayer;
        
        internal System.Windows.Controls.Canvas OverlayRect;
        
        internal System.Windows.Controls.ProgressBar BufferProgress;
        
        internal System.Windows.Controls.Canvas UnderlayRect;
        
        private bool _contentLoaded;
        
        /// <summary>
        /// InitializeComponent
        /// </summary>
        [System.Diagnostics.DebuggerNonUserCodeAttribute()]
        public void InitializeComponent() {
            if (_contentLoaded) {
                return;
            }
            _contentLoaded = true;
            System.Windows.Application.LoadComponent(this, new System.Uri("/Starlight.SamplePlayer;component/Page.xaml", System.UriKind.Relative));
            this.ParentCanvas = ((System.Windows.Controls.Canvas)(this.FindName("ParentCanvas")));
            this.PlayerGrid = ((System.Windows.Controls.Grid)(this.FindName("PlayerGrid")));
            this.StatusText = ((System.Windows.Controls.TextBlock)(this.FindName("StatusText")));
            this.BufferLevelText = ((System.Windows.Controls.TextBlock)(this.FindName("BufferLevelText")));
            this.PositionText = ((System.Windows.Controls.TextBlock)(this.FindName("PositionText")));
            this.MediaPlayer = ((System.Windows.Controls.MediaElement)(this.FindName("MediaPlayer")));
            this.OverlayRect = ((System.Windows.Controls.Canvas)(this.FindName("OverlayRect")));
            this.BufferProgress = ((System.Windows.Controls.ProgressBar)(this.FindName("BufferProgress")));
            this.UnderlayRect = ((System.Windows.Controls.Canvas)(this.FindName("UnderlayRect")));
        }
    }
}
