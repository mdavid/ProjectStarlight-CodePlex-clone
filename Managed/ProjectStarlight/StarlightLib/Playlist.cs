﻿#region License Header
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
using System.Net;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Shapes;

namespace Starlight.Lib
{
    /// <summary>
    /// Holds a collection of playlist entries.  When associated with a MediaElement, this
    /// playlist will switch the MediaElement's Source property based on MediaEnded and 
    /// MediaFailed events received from the MediaElement.
    /// </summary>
    public class Playlist
    {
        private PlaylistEntry[] playlistEntries;
        private int playlistEntryIndex = -1;
        private MediaElement player;
        private Dictionary<string, object> bridgeContext;
        private bool playOnOpen = false;

        public Playlist(PlaylistEntry[] playlistEntries)
        {
            this.playlistEntries = playlistEntries;
            foreach (PlaylistEntry pe in playlistEntries)
            {
                pe.Playlist = this;
            }
        }

        public PlaylistEntry[] PlaylistEntries
        {
            get
            {
                return playlistEntries;
            }
        }

        public PlaylistEntry CurrentEntry
        {
            get
            {
                if (playlistEntryIndex < 0 || playlistEntryIndex > playlistEntries.Length - 1)
                {
                    return null;
                }
                return playlistEntries[playlistEntryIndex];
            }
        }

        public event EventHandler EntryChanged;
        public event EventHandler PlaylistComplete;


        /// <summary>
        /// Runs this playlist in the associated media element.
        /// </summary>
        public void Play(Dictionary<string, object> bridgeContext)
        {
            this.player.AutoPlay = false;
            playOnOpen = true;
            this.bridgeContext = bridgeContext;
            playlistEntryIndex = 0;
            playlistEntries[playlistEntryIndex].SwitchTo(bridgeContext, player);
            OnEntryChanged();
        }

        public void Stop(Dictionary<string, object> bridgeContext)
        {
            if (playlistEntryIndex < playlistEntries.Length)
            {
                playlistEntries[playlistEntryIndex].Leaving(bridgeContext, this.player);
            }
            playlistEntryIndex = playlistEntries.Length;
            this.player.Stop();
        }

        /// <summary>
        /// Associates this playlist with a MediaElement.  The MediaElement will play all entries in
        /// this playlist when it is started.
        /// </summary>
        /// <param name="player"></param>
        public void Associate(MediaElement player)
        {
            this.player = player;
            player.MediaOpened += new RoutedEventHandler(OnMediaOpen);
            player.MediaEnded += new RoutedEventHandler(OnMediaEnd);
            player.MediaFailed += new EventHandler<ExceptionRoutedEventArgs>(OnMediaError);
        }

        public void OnEntryChanged()
        {
            if (EntryChanged != null)
            {
                this.EntryChanged(this, EventArgs.Empty);
            }
        }

        public void OnEntryEnded(PlaylistEntry entry)
        {
            if(CurrentEntry != null && entry != null && CurrentEntry.Contains(entry))
            {
                playlistEntries[playlistEntryIndex].Leaving(bridgeContext, player);
                playlistEntryIndex++;
                if (playlistEntryIndex < playlistEntries.Length)
                {
                    playlistEntries[playlistEntryIndex].SwitchTo(bridgeContext, player);
                    OnEntryChanged();
                }
                else
                {
                    if (PlaylistComplete != null)
                    {
                        this.PlaylistComplete(this, null);
                    }
                }
            }
        }

        private void OnMediaEnd(object o, RoutedEventArgs args)
        {
            OnEntryEnded(CurrentEntry);
        }

        private void OnMediaError(object o, ExceptionRoutedEventArgs args)
        {
            if (playlistEntryIndex < playlistEntries.Length) 
            {
                if (!playlistEntries[playlistEntryIndex].FailOver(bridgeContext, player))
                {
                    OnEntryEnded(CurrentEntry);
                }
            }
        }

        private void OnMediaOpen(object o, RoutedEventArgs args)
        {
            if (playOnOpen)
            {
                player.Play();
                player.AutoPlay = true;
                playOnOpen = false;
            }
        }
    }
}
