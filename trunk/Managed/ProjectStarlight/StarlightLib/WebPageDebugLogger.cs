using System;
using System.Net;
using System.Windows;
using System.Windows.Browser;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace Starlight.Lib
{
    public class WebPageDebugLogger : Starlight.ASF.IDebugLogger
    {
        private HtmlElement element;
        private Dispatcher dispatcher;

        public WebPageDebugLogger(HtmlElement element)
        {
            this.element = element;
            this.dispatcher = element.Dispatcher;
        }

        public void WriteDebug(string message)
        {
            dispatcher.BeginInvoke(delegate()
            {
                string html = element.GetAttribute("innerHTML");
                html += "<div><pre>" + message + "</pre></div>";
                element.SetAttribute("innerHTML", html);
            });
        }
    }
}
