#region License Header
/**
* 
* This license governs use of the accompanying software. If you use the software, you
* accept this license. If you do not accept the license, do not use the software.
*
* 1. Definitions
* The terms "reproduce," "reproduction," "derivative works," and "distribution" have the
* same meaning here as under U.S. copyright law.
* A "contribution" is the original software, or any additions or changes to the software.
* A "contributor" is any person that distributes its contribution under this license.
* "Licensed patents" are a contributor's patent claims that read directly on its contribution.
* 
* 2. Grant of Rights
* (A) Copyright Grant- Subject to the terms of this license, including the license conditions and limitations in section 3, each contributor grants you a non-exclusive, worldwide, royalty-free copyright license to reproduce its contribution, prepare derivative works of its contribution, and distribute its contribution or any derivative works that you create.
* (B) Patent Grant- Subject to the terms of this license, including the license conditions and limitations in section 3, each contributor grants you a non-exclusive, worldwide, royalty-free license under its licensed patents to make, have made, use, sell, offer for sale, import, and/or otherwise dispose of its contribution in the software or derivative works of the contribution in the software.
* 
* 3. Conditions and Limitations
* (A) No Trademark License- This license does not grant you rights to use any contributors' name, logo, or trademarks.
* (B) If you bring a patent claim against any contributor over patents that you claim are infringed by the software, your patent license from such contributor to the software ends automatically.
* (C) If you distribute any portion of the software, you must retain all copyright, patent, trademark, and attribution notices that are present in the software.
* (D) If you distribute any portion of the software in source code form, you may do so only under this license by including a complete copy of this license with your distribution. If you distribute any portion of the software in compiled or object code form, you may only do so under a license that complies with this license.
* (E) The software is licensed "as-is." You bear the risk of using it. The contributors give no express warranties, guarantees or conditions. You may have additional consumer rights under your local laws which this license cannot change. To the extent permitted under your local laws, the contributors exclude the implied warranties of merchantability, fitness for a particular purpose and non-infringement.
*
*/
#endregion

using System;
using System.Collections.Generic;
using System.Net;
using System.Windows;
using System.Windows.Threading;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Shapes;
using System.Windows.Browser;
using Starlight.ASF;

namespace Starlight.Lib
{
    /// <summary>
    /// This class is the entry point to the Multicast library to be called from
    /// javascript in the web page.  To use this class, you must attach it to a 
    /// media element using the AttachMediaElement() method.  Once it is attached,
    /// a playlist must be loaded, and then StartStreaming called.
    /// 
    /// </summary>
    [ScriptableType]
    public class MulticastController
    {
        public static readonly string KEY_PUSH_SOURCE_CONTROLLER = "KEY_PUSH_SOURCE_CONTROLLER";
        public static readonly string KEY_BRIDGE = "KEY_BRIDGE";

        private Dictionary<string, object> bridgeContext = new Dictionary<string, object>();
        private Playlist playlist;
        private MediaElement mediaPlayer;
        private Dispatcher uiThreadDispatcher;

        public MulticastController()
        {
            
        }

        public event EventHandler PlaylistLoaded;

        public uint CurrentPlayerTime
        {
            get
            {
                return (uint)mediaPlayer.Position.Ticks;
            }
        }

        public Playlist Playlist
        {
            get { return playlist; }
        }

        protected Dispatcher UIThreadDispatcher
        {
            get { return uiThreadDispatcher; }
        }

        protected virtual PlaylistParserFactory PlaylistParserFactory
        {
            get { return new PlaylistParserFactory(); }
        }

        /// <summary>
        /// Attaches this controller to a given media element.  Once attached,
        /// this controller will drive the media element, switching sources and
        /// playing according to the playlist specified.
        /// </summary>
        /// <param name="mediaElement"></param>
        public void AttachMediaElement(MediaElement mediaElement)
        {
            this.mediaPlayer = mediaElement;
            this.uiThreadDispatcher = mediaElement.Dispatcher;
        }

        /// <summary>
        /// Starts loading the playlist at the given playlist url.  The PlaylistLoaded event will be signaled
        /// when loading of the playlist is complete.
        /// 
        /// </summary>
        /// <param name="playlistUrl"></param>
        public void LoadPlaylistAsync(string playlistUrl)
        {
            if(playlistUrl.StartsWith("asx:"))
            {
                string playlistContent = playlistUrl.Substring(4);
                DoParsePlaylist(playlistContent);
            }
            else if (playlistUrl.EndsWith(".nsc"))
            {
                PlaylistEntry[] entries = new PlaylistEntry[1];
                entries[0] = new NSCPlaylistEntry(playlistUrl);
                Playlist playlist = new Playlist(entries);
                OnParsePlaylistComplete(playlist);
            }
            else
            {
                WebClient webclient = new WebClient();
                webclient.DownloadStringCompleted += new DownloadStringCompletedEventHandler(OnDownloadPlaylistCompleted);
                webclient.DownloadStringAsync(new Uri(playlistUrl));
            }
        }

        /// <summary>
        /// Starts streaming from the provided playlist.  Should only be called after the PlaylistLoaded
        /// event is signaled.
        /// </summary>
        /// <param name="proxy"></param>
        public void StartStreaming(ScriptObject proxy)
        {
            if (playlist == null)
            {
                throw new Exception("Playlist not yet loaded");
            }
            InitBridgeContext(bridgeContext, proxy);
            this.playlist.Play(bridgeContext);

        }

        protected virtual void InitBridgeContext(Dictionary<string, object> bridgeContext, ScriptObject proxy)
        {
            bridgeContext.Add(KEY_PUSH_SOURCE_CONTROLLER, new ScriptObjectPushSourceController(proxy));
            bridgeContext.Add(KEY_BRIDGE, this);
        }

        private void OnDownloadPlaylistCompleted(object sender, DownloadStringCompletedEventArgs e)
        {
            DoParsePlaylist(e.Result);
        }

        private void DoParsePlaylist(string playlistContent)
        {
            PlaylistParserFactory parserFactory = this.PlaylistParserFactory;
            PlaylistParser parser = parserFactory.CreateParser(playlistContent);
            parser.ParsePlaylistCompleted += new PlaylistParser.OnParsePlaylistCompletedEventHandler(OnParsePlaylistComplete);
            parser.ParsePlaylistAsync(playlistContent);
        }

        private void OnParsePlaylistComplete(Playlist playlist)
        {
            this.playlist = playlist;
            playlist.Associate(mediaPlayer);
            this.PlaylistLoaded(this, null);
        }
    }
}
