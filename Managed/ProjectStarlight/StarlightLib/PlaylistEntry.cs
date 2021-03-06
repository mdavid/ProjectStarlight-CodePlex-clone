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
using System.Collections.Generic;
using System.Net;
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
    /// A single entry in a playlist.
    /// </summary>
    public abstract class PlaylistEntry
    {
        public virtual Playlist Playlist{ get; set;}

        /// <summary>
        /// Initializes the playlist entry.  This may require access to remote resources,
        /// so is done aynchronously.
        /// </summary>
        public abstract void InitAsync();

        /// <summary>
        /// Switches this player given by MediaElement to use this playlist entry.
        /// </summary>
        /// <param name="mediaElement"></param>
        public abstract void SwitchTo(Dictionary<string, object> bridgeContext, MediaElement mediaElement);

        /// <summary>
        /// Called when the stream associated with this playlist entry fails.
        /// </summary>
        /// <param name="mediaElement"></param>
        /// <returns>a bool indicating whether failover successfully occured.</returns>
        public virtual bool FailOver(Dictionary<string, object> bridgeContext, MediaElement mediaElement)
        {
            return false;
        }

        public virtual bool Contains(PlaylistEntry entry)
        {
            return entry == this;
        }

        /// <summary>
        /// Called when switching away from this playlist entry to allow any cleanup needed to occur.
        /// </summary>
        /// <param name="bridgeContext"></param>
        /// <param name="mediaElement"></param>
        public virtual void Leaving(Dictionary<string, object> bridgeContext, MediaElement mediaElement)
        {
        }

        protected void OnInitCompleted()
        {
            this.InitCompleted();
        }

        public delegate void OnInitCompletedEventHandler();

        /// <summary>
        /// Called when initialization of this playlist entry is completed.
        /// </summary>
        public event OnInitCompletedEventHandler InitCompleted;
    }
}
