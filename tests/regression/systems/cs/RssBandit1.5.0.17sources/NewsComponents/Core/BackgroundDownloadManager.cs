#region CVS Version Header
/*
 * $Id: BackgroundDownloadManager.cs,v 1.16 2007/07/21 12:26:21 t_rendelmann Exp $
 * Last modified by $Author: t_rendelmann $
 * Last modified at $Date: 2007/07/21 12:26:21 $
 * $Revision: 1.16 $
 */
#endregion

using System;
using System.IO;
using System.Collections;
using System.Net;
using System.Runtime.Serialization;
using System.Security.Permissions;
using NewsComponents.Net;
using NewsComponents.Resources;
using NewsComponents.Utils;

namespace NewsComponents
{

	/// <summary>
	/// Provides some of the file independent information needed to download a file. 
	/// </summary>
	public interface IDownloadInfoProvider{
	
		/// <summary>
		/// The proxy that must be used when accessing the Web
		/// </summary>
		IWebProxy Proxy{get;}  

		/// <summary>
		/// Retrieves the target folder for a particular DownloadItem
		/// </summary>
		/// <param name="item">The item being downloaded</param>
		/// <returns>The target folder to download the file to</returns>
		string GetTargetFolder(DownloadItem item);

		/// <summary>
		/// Retrieves the credentials needed for accessing a particular DownloadItem
		/// </summary>
		/// <param name="item">The item being dowbloaded</param>
		/// <returns>The credentials needed to access the item</returns>
		ICredentials GetCredentials(DownloadItem item);
		
		/// <summary>
		/// This is the location where BITS should temporarily download files until moving them to 
		/// TargetFolder on completion. 
		/// </summary>
		/// <returns></returns>
		string InitialDownloadLocation{get;}

	}

	/// <summary>
	/// Public class that client application uses to operate with the 
	/// background download manager.
	/// </summary>
	public class BackgroundDownloadManager: IDownloadInfoProvider
	{
		#region private vars

		/// <summary>
		/// Used for making asynchronous Web requests by HttpDownloader instances
		/// </summary>
		private static AsyncWebRequest asyncWebRequest = null;
	
		/// <summary>
		/// The applicationID the updater instance is for.
		/// </summary>
		private string applicationId;

		/// <summary>
		/// This is used to provide information about the files being downloaded 
		/// such as target directory and credentials. 
		/// </summary>
		private NewsHandler downloadInfoProvider = null;
		
		/// <summary>
		/// Key in the task context.
		/// </summary>
		private const string AsyncTaskKey = "IsAsyncTask";
	

		/// <summary>
		/// Maximum time to wait when resuming a pending update
		/// </summary>
		private TimeSpan downloadResumeMaxWaitTime = TimeSpan.FromMinutes( 10 );

		#endregion

		#region Public Properties


		/// <summary>
		/// This is the location where BITS should temporarily download files until moving them to 
		/// TargetFolder on completion. 
		/// </summary>
		/// <returns></returns>
		public string InitialDownloadLocation{
			get{
				return this.downloadInfoProvider.CacheLocation;
			}
		}

		/// <summary>
		/// The proxy that must be used when accessing the Web
		/// </summary>
		public IWebProxy Proxy{
			get{
				return this.downloadInfoProvider.Proxy; 
			}
		}

		#endregion
		
		#region Events

		
		/// <summary>
		/// Notifies downloads have started.
		/// </summary>
		public event DownloadStartedEventHandler DownloadStarted;

		/// <summary>
		/// Notifies download progress.
		/// </summary>
		public event DownloadProgressEventHandler DownloadProgress;

		/// <summary>
		/// Notifies download errors.
		/// </summary>
		public event DownloadErrorEventHandler DownloadError;

		/// <summary>
		/// Notifies that download is complete.
		/// </summary>
		public event DownloadCompletedEventHandler DownloadCompleted;
		

		#endregion

		#region ctor
		/// <summary>
		/// Initializes a new instance of the <see cref="BackgroundDownloadManager"/> class.
		/// </summary>
		/// <param name="configuration">The configuration. Used: ApplicationID and UserLocalApplicationDataPath</param>
		/// <param name="downloadInfoProvider">The IDownloadInfoProvider instance. It is used to 
		/// request the download informations like proxy or credentials at the time the real
		/// download is queued.</param>
		public BackgroundDownloadManager(INewsComponentsConfiguration configuration, NewsHandler downloadInfoProvider) {
			this.applicationId = configuration.ApplicationID;
			this.downloadInfoProvider = downloadInfoProvider;
			
			DownloadRegistryManager.Current.SetBaseFolder(configuration.UserLocalApplicationDataPath);
		}
		
		/// <summary>
		/// Constructor initializes class.
		/// </summary>
		/// <param name="applicationName">The Application Name or ID that uses the component. 
		/// This will be used to initialize the user path to store the 
		/// download registry.</param>
		/// <param name="downloadInfoProvider">The IDownloadInfoProvider instance. It is used to 
		/// request the download informations like proxy or credentials at the time the real
		/// download is queued.</param>
		public BackgroundDownloadManager(string applicationName, NewsHandler downloadInfoProvider)
		{
			this.applicationId = applicationName;
			this.downloadInfoProvider = downloadInfoProvider;
			
			DownloadRegistryManager.Current.SetBaseFolder(GetUserPath(applicationId));
		}


		/// <summary>
		/// Static constructor
		/// </summary>
		static BackgroundDownloadManager()	{	
			asyncWebRequest = new AsyncWebRequest(); 
		}
	
		#endregion

		#region Static Members

		/// <summary>
		/// Used for making asynchronous web requests
		/// </summary>
		public static AsyncWebRequest AsyncWebRequest{
			get{
				return asyncWebRequest;
			}
		}

		/// <summary>
		/// Returns the user path used to store the current feed and cached items.
		/// </summary>
		/// <param name="appname">The application name that uses the component.</param>
		/// <returns></returns>
		private static string GetUserPath(string appname) {
			string s = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), appname);
			if(!Directory.Exists(s)) Directory.CreateDirectory(s);
			return s;
		}

		
		/// <summary>
		/// Retrieves the value of various HTTP headers from the file using an HTTP HEAD request.
		/// </summary>
		/// <param name="file">The file.</param>
		/// <param name="credentials">The credentials.</param>
		/// <param name="proxy">The proxy server to send requests through</param>
		/// <remarks>Relevant HTTP Headers are: Content-Length, Content-Type and Content-Diposition</remarks>
		/// <returns>A hashtable containing entries for the Content-Length, Content-Type and Content-Diposition headers.</returns>
		public static Hashtable GetRelevantHttpHeaders(DownloadFile file, ICredentials credentials, IWebProxy proxy) {
					

				Hashtable headers = new Hashtable(); 

			if (file == null || StringHelper.EmptyTrimOrNull(file.Source))
				return headers;
				
				if (file.Source.StartsWith("http")){
														
					HttpWebRequest request = (HttpWebRequest)WebRequest.Create(file.Source);
					if (credentials != null)
						request.Credentials = credentials;
					request.AllowAutoRedirect = true;
					request.Method = "HEAD";
					request.Proxy  = proxy;

					WebResponse response = request.GetResponse();
					if (response != null) {
						headers.Add("Content-Length", response.ContentLength); 
						headers.Add("Content-Disposition", response.Headers["Content-Disposition"]); 						
						headers.Add("Content-Type", response.ContentType);
						response.Close();						
					}
				}
			
			
			return headers;
		}

		#region Registry Management Methods

		/// <summary>
		/// Returns the list of registered tasks.
		/// </summary>
		/// <returns>A list of tasks corresponding to downloads currently in progress.</returns>
		public static DownloadTask[] GetTasks() {
			return DownloadRegistryManager.Current.GetTasks();
		}

		#endregion

		#endregion


		#region Private methods


		/// <summary>
		/// Return the downloader instance for a specific DownloadTask.
		/// </summary>
		/// <param name="task">The DownloadTask.</param>
		/// <returns>The Downloader instance.</returns>
		private IDownloader GetDownloader( DownloadTask task ) {
 			
			IDownloader downloader = null;

			if(task.Downloader != null){
				return task.Downloader; 
			}
			
			bool contentLengthSpecified = UpdateTaskFromHttpHeaders(task); 				

			//We only support HTTP and HTTPS
			if (IsOSAtLeastWindowsXP && !FileHelper.IsUncPath(task.DownloadItem.File.Source)){

				/* If no Content-Length then use HttpDownloader since BITS can't handle files 
				 * without Content-Length specified 
				 */
				if(contentLengthSpecified){
					downloader = new BITSDownloader();	
				}else if(task.DownloadItem.Enclosure.Length <= (15 * 1024 * 1024)){ 
					//To avoid consuming excess resources we limit direct HTTP downloads to no greater than 15MB 
					//See http://blogs.msdn.com/rssteam/archive/2006/12/06/enclosure-download.aspx for more details
					downloader = new HttpDownloader();	
				}
			
				if(downloader != null){
					downloader.DownloadStarted += new DownloadTaskStartedEventHandler( OnDownloadStarted );
					downloader.DownloadProgress += new DownloadTaskProgressEventHandler( OnDownloadProgress );
					downloader.DownloadCompleted += new DownloadTaskCompletedEventHandler( OnDownloadCompleted );
					downloader.DownloadError += new DownloadTaskErrorEventHandler( OnDownloadError );				
				}
			}

			task.Downloader = downloader; 
			return downloader;
		}


		/// <summary>
		/// Returns the most accurate size we have determined for the DownloadTask or -1 if no size information
		/// is available. 
		/// </summary>
		/// <param name="task">The download task</param>
		/// <returns>The most accurate size we have determined for the DownloadTask or -1 if no size information
		/// is available</returns>
		private static long GetFileSize(DownloadTask task){

			if(task.DownloadItem.File.FileSize > 0){
				return task.DownloadItem.File.FileSize; 		
			}else if(task.DownloadItem.Enclosure.Length > 0){
				return task.DownloadItem.Enclosure.Length;
			}else{
				return -1;
			}
		
		}

		/// <summary>
		/// Updates the information about the download task based on performing an HTTP HEAD 
		/// request on the file to download. 
		/// </summary>
		/// <param name="task">The download task to update</param>
		/// <returns>True if the Content-Length header was specified</returns>
		private static bool UpdateTaskFromHttpHeaders(DownloadTask task){
		
			Hashtable   headers    = null;
			long contentLength     = task.DownloadItem.Enclosure.Length;

			/* 
			 * 	1. Set filename using Content-Disposition header value if it exists
			 *  2. Get size from Content-Length header if it exists
			 *  3. Get MIME type from Content-Type header if it exists
			 */ 
			try{ 
				headers = GetRelevantHttpHeaders(task.DownloadItem.File,  task.DownloadItem.Credentials, task.DownloadItem.Proxy);								

				contentLength = (long) headers["Content-Length"]; 					
				if(contentLength > 0){
					task.DownloadItem.File.FileSize = contentLength; 
				}

				if(!StringHelper.EmptyOrNull(headers["Content-Type"] as string)){
					task.DownloadItem.File.SuggestedType = new MimeType(headers["Content-Type"].ToString());
				}					
				
				if(!StringHelper.EmptyOrNull(headers["Content-Disposition"] as string)){
					string[] components = headers["Content-Disposition"].ToString().Split(new char[]{';'});
					
					foreach(string s in components){
						string str = s.Trim(); 						
						
						if(str.StartsWith("filename=") && str.Length > 9){
							task.DownloadItem.File.LocalName = str.Substring(9);
						}
					}
				}

			}catch{;}

			return (contentLength > 0); 
		}

		/// <summary>
		/// Checks if any file is a UNC path and return true in case,
		/// else false 
		/// </summary>
		/// <param name="files">DownloadFilesCollection instance or null</param>
		/// <returns>bool</returns>
		private bool AnyFileIsUncPath(DownloadFilesCollection files) {
			if (files == null) return false;
			foreach (DownloadFile f in files)
				if (FileHelper.IsUncPath(f.Source))
					return true;
			return false;
		}
		

		/// <summary>
		/// Returns true, if the OS is at least Windows XP (or higher), else false.
		/// </summary>
		private bool IsOSAtLeastWindowsXP {
			get { 
				return (Environment.OSVersion.Platform == PlatformID.Win32NT && (Environment.OSVersion.Version.Major > 5 || (Environment.OSVersion.Version.Major == 5 && Environment.OSVersion.Version.Minor >= 1)));
			}
		}
	

		#endregion

		#region Public Methods			


		

		/// <summary>
		/// Retrieves the target folder for a particular DownloadItem
		/// </summary>
		/// <param name="item">The item being downloaded</param>
		/// <returns>The target folder to download the file to</returns>
		public string GetTargetFolder(DownloadItem item){
		
			return this.downloadInfoProvider.GetEnclosureFolder(item.OwnerFeedId, item.File.LocalName);
		}

		/// <summary>
		/// Retrieves the credentials needed for accessing a particular DownloadItem
		/// </summary>
		/// <param name="item">The item being dowbloaded</param>
		/// <returns>The credentials needed to access the item</returns>
		public ICredentials GetCredentials(DownloadItem item){
		
			return this.downloadInfoProvider.GetFeedCredentials(item.OwnerFeedId); 
		}		

		/// <summary>
		/// Synchronously submits a task to the downloader.
		/// </summary>
		/// <param name="task">The DownloadTask instance.</param>
		/// <param name="maxWaitTime">The maximum wait time for the task.</param>
		private void SubmitTask(DownloadTask task, TimeSpan maxWaitTime) {
		
			IDownloader downloader = GetDownloader( task );
		
			// If there's no files to download
			// Consider the download as done
			if((downloader == null) || ( task.DownloadItem.File == null )) {
				OnDownloadCompleted( null, new TaskEventArgs( task ) );
				return;
			}
					
			try {
				downloader.Download( task, maxWaitTime );
			}
			finally {
				Release(downloader);
			}
			
		}

		/// <summary>
		/// Asynchronously submits a task to the downloader.
		/// </summary>
		/// <param name="task">The DownloadTask instance.</param>
		private void SubmitTaskAsync( DownloadTask task ) {		

			IDownloader downloader = GetDownloader( task );

			
			if(this.downloadInfoProvider.EnclosureCacheSize != Int32.MaxValue){
			
				//BUGBUG: If we don't know the file size then we don't error because we treat it is -1 to 
				//err on the side of allowing instead of disallowing. 
				long limitInBytes = this.downloadInfoProvider.EnclosureCacheSize * 1024 * 1024; 
				long filesize   = GetFileSize(task); 
				
				DirectoryInfo targetDir = new DirectoryInfo(this.downloadInfoProvider.EnclosureFolder); 
				long spaceUsed  = FileHelper.GetSize(targetDir);				

				if(!this.downloadInfoProvider.EnclosureFolder.Equals(this.downloadInfoProvider.PodcastFolder)){
					DirectoryInfo podcastDir = new DirectoryInfo(this.downloadInfoProvider.EnclosureFolder); 
					spaceUsed  += FileHelper.GetSize(podcastDir);				
				}

				if( (filesize + spaceUsed) > limitInBytes){
					DownloadRegistryManager.Current.UnRegisterTask( task );	

					string fileName = task.DownloadItem.File.LocalName; 
					int limit = this.downloadInfoProvider.EnclosureCacheSize; 							
					throw new DownloaderException(ComponentsText.ExceptionEnclosureCacheLimitReached(fileName, limit));				
				}
			}


			// If there's no files to download
			// Consider the download as done
			if((downloader == null) || ( task.DownloadItem.File == null )) {
				OnDownloadCompleted( null, new TaskEventArgs( task ) );
			}else{
				downloader.BeginDownload( task );
			}
			
		}

		/// <summary>
		/// Cancels a pending task.
		/// </summary>
		/// <param name="task">The DownloadTask instance.</param>
		/// <returns></returns>
		public bool EndTask( DownloadTask task ) {
			IDownloader downloader = GetDownloader( task );
			bool success = true; 

			if(downloader != null){

				try {
					success = downloader.CancelDownload( task );
				}
				finally {
					Release(downloader);
				}
			}//if(downloader != null) 

			return success; 
		}

		

		/// <summary>
		/// Resumes pending downloads that were not completed in a 
		/// previous session. 
		/// </summary>
		public void ResumePendingDownloads() {
			// If there are pending tasks for this application, resume them
			// accordingly to their state
			foreach(DownloadTask task in DownloadRegistryManager.Current.GetTasks() ) {
				switch( task.State ) {
					case DownloadTaskState.DownloadError:
					case DownloadTaskState.Downloading:
						task.Init(task.DownloadItem, this);
						SubmitTaskAsync( task );
						break;
					case DownloadTaskState.Downloaded:
						// Unregister the task if we somehow missed it on previous run
						DownloadRegistryManager.Current.UnRegisterTask( task );
						break;
				}
			}
			
		}


		/// <summary>
		/// Cancels pending downloads.
		/// </summary>
		/// <param name="ownerId">The ID of the feed that the task belongs to</param>
		public void CancelPendingDownloads(string ownerId) {
			DownloadTask[] pendingTasks = DownloadRegistryManager.Current.GetByOwnerId(ownerId);
			foreach(DownloadTask task in pendingTasks) {
				this.GetDownloader(task).CancelDownload(task);
				DownloadRegistryManager.Current.UnRegisterTask( task );				
			}
		}


		/// <summary>
		/// Cancels pending downloads.
		/// </summary>
		public void CancelPendingDownloads() {
			DownloadTask[] pendingTasks = DownloadRegistryManager.Current.GetTasks();
			foreach(DownloadTask task in pendingTasks) {
				this.GetDownloader(task).CancelDownload(task);
				DownloadRegistryManager.Current.UnRegisterTask( task );
			}
		}

		/// <summary>
		/// Synchronously downloads the specified files in specified 
		/// DownloadItem array.
		/// </summary>
		/// <param name="items">The list of DownloadItems to process.</param>
		/// <param name="maxWaitTime">The maximum amount of time for download to complete before timeout.</param>
		public void Download( DownloadItem[] items, TimeSpan maxWaitTime ) {
			foreach(DownloadItem item in items) {
				Download(item, maxWaitTime);
			}		
		}

		/// <summary>
		/// Synchronously downloads the specified files in specified 
		/// DownloadItem.
		/// </summary>
		/// <param name="item">The DownloadItem to process.</param>
		/// <param name="maxWaitTime">The maximum amount of time for download to complete before timeout.</param>
		public void Download( DownloadItem item, TimeSpan maxWaitTime ) {
			DownloadTask task = new DownloadTask( item, this );
			
			if(DownloadRegistryManager.Current.TaskAlreadyExists(task)){
				DownloadRegistryManager.Current.RegisterTask( task );
				task[ AsyncTaskKey ] = false;
				SubmitTask( task, maxWaitTime );
			}
		}

		/// <summary>
		/// Asynchronously begins a download of the files specified 
		/// in DownloadItem array.
		/// </summary>
		/// <param name="items">The list of DownloadItems to process.</param>
		public void BeginDownload( DownloadItem[] items ) {
			foreach(DownloadItem item in items) {
				BeginDownload(item);
			}		
		}
		/// <summary>
		/// Asynchronously begins a download of the files specified 
		/// in DownloadItem.
		/// </summary>
		/// <param name="item">The DownloadItem to process.</param>
		public void BeginDownload( DownloadItem item ) {
			DownloadTask task = new DownloadTask( item, this );

			if(!DownloadRegistryManager.Current.TaskAlreadyExists(task)){			
				DownloadRegistryManager.Current.RegisterTask( task );
				task[ AsyncTaskKey ] = true;
				SubmitTaskAsync( task ); 
			}
		}

		/// <summary>
		/// Cancels the asynchronous download operation associated
		/// with the specified DownloadItem. 
		/// </summary>
		/// <exception cref="InvalidOperationException">Thrown if the related update 
		/// task is in a state that is not cancelable.</exception>
		/// <param name="item">The DownloadItem instance to cancel.</param>
		public void CancelDownload( DownloadItem item ) {
			DownloadTask task = DownloadRegistryManager.Current.GetByItemID( item.ItemId );
			if ( task != null ) {
				switch(	task.State ) {
					case DownloadTaskState.None: {
						DownloadRegistryManager.Current.UnRegisterTask( task );
						break;
					}
					case DownloadTaskState.DownloadError:
					case DownloadTaskState.Downloading: {
						EndTask( task );
						DownloadRegistryManager.Current.UnRegisterTask( task );
						break;
					}
					case DownloadTaskState.Downloaded: {
						DownloadRegistryManager.Current.UnRegisterTask( task );
						break;
					}
				}

				lock(task.SyncRoot) {
					task.State = DownloadTaskState.Cancelled;		
				}

			}
		}

		#endregion

		#region Event subscription, handling and forwarding methods

		/// <summary>
		/// Method used to handle the event.
		/// </summary>
		/// <param name="sender">The sender of the event.</param>
		/// <param name="e">The task information.</param>
		private void OnDownloadCompleted(object sender, TaskEventArgs e) {
			
			string fileLocation = Path.Combine(e.Task.DownloadFilesBase, e.Task.DownloadItem.File.LocalName);
			string finalLocation = Path.Combine(e.Task.DownloadItem.TargetFolder, e.Task.DownloadItem.File.LocalName); 
			
			/*
				e.Task.State = DownloadTaskState.Downloaded;	
				DownloadRegistryManager.Current.UpdateTask(e.Task); 
			 */ 

			//TODO: Once we have a UI for managing enclosures we'll need to keep the task around 			
			DownloadRegistryManager.Current.UnRegisterTask( e.Task );		

			try{
				/* 
				 * Add Zone.Identifier to File to indicate that the file was downloaded from the Web
				 * See http://geekswithblogs.net/ssimakov/archive/2004/08/17/9805.aspx for more details
				 */
				FileStreams FS = new FileStreams(fileLocation);

				//Remove Zone.Identifier if it already exists since we can't trust it
				//Not sure if this can happen. 
				int i = FS.IndexOf("Zone.Identifier");
				if (i!= -1){
					FS.Remove("Zone.Identifier"); 
				} 

				FS.Add("Zone.Identifier");
				FileStream fs = FS["Zone.Identifier"].Open(System.IO.FileMode.OpenOrCreate, System.IO.FileAccess.Write);
				StreamWriter writer = new StreamWriter(fs); 
				writer.WriteLine("[ZoneTransfer]");
				writer.WriteLine("ZoneId=3");
				writer.Close();
				fs.Close();

				if(!Directory.Exists(e.Task.DownloadItem.TargetFolder)){
					Directory.CreateDirectory(e.Task.DownloadItem.TargetFolder); 
				}
		
				/* Move file to TargetFolder from temporary download location*/
				FileHelper.MoveFile(fileLocation, finalLocation, MoveFileFlag.CopyAllowed | MoveFileFlag.ReplaceExisting); 

				/* Initiate callback to waiting callers */ 
				if ( DownloadCompleted != null ) {
					DownloadCompleted( this, new DownloadItemEventArgs( e.Task.DownloadItem ) );
				}
			
				if ( sender is IDownloader ) {
					IDownloader downloader = (IDownloader)sender;
					Release(downloader);
				}

			}catch(Exception error){
				this.OnDownloadError(this, new DownloadTaskErrorEventArgs(e.Task,error));		
				return;
			}
									
		}

		/// <summary>
		/// Method used to handle the event.
		/// </summary>
		/// <param name="sender">The sender of the event.</param>
		/// <param name="e">The download error information.</param>
		private void OnDownloadError( object sender, DownloadTaskErrorEventArgs e ) {
			/* 
				e.Task.State = DownloadTaskState.DownloadError;					
				DownloadRegistryManager.Current.UpdateTask( e.Task );
			*/

			//TODO: Once we have a UI for managing enclosures we'll need to keep the task around 			
			DownloadRegistryManager.Current.UnRegisterTask( e.Task );	

			if ( DownloadError != null ) {
				DownloadError( this, new DownloadItemErrorEventArgs( e.Task.DownloadItem, e.Exception ) );
			}
			IDownloader downloader = (IDownloader)sender;
			Release(downloader);
		}

		/// <summary>
		/// Method used to handle the event.
		/// </summary>
		/// <param name="sender">The sender of the event.</param>
		/// <param name="e">The download progress task information.</param>
		private void OnDownloadProgress(object sender, DownloadTaskProgressEventArgs e) {
			if ( DownloadProgress != null ) {

				DownloadProgressEventArgs eventArgs = new DownloadProgressEventArgs( 
					e.BytesTotal,
					e.BytesTransferred, 
					e.FilesTotal, 
					e.FilesTransferred, 
					e.Task.DownloadItem );
				DownloadProgress( this, eventArgs );
				if ( eventArgs.Cancel ) {
					CancelDownload( e.Task.DownloadItem );
				}
			}
		}

		/// <summary>
		/// Method used to handle the event.
		/// </summary>
		/// <param name="sender">The sender of the event.</param>
		/// <param name="e">The task information.</param>
		private void OnDownloadStarted(object sender, TaskEventArgs e ) {

			e.Task.State = DownloadTaskState.Downloading;			
			DownloadRegistryManager.Current.UpdateTask( e.Task );

			if ( DownloadStarted != null ) {				
				DownloadStartedEventArgs eventArgs = new DownloadStartedEventArgs( e.Task.DownloadItem );
				DownloadStarted( this, eventArgs );
				if ( eventArgs.Cancel ) {
					CancelDownload( eventArgs.DownloadItem );
				}
			}
		}
		

		
			

	

		/// <summary>
		/// Method to unregister Downloader events and invoke Dispose() on the downloader.
		/// </summary>
		/// <param name="downloader">
		/// Downloader instance to unregister events from.
		/// </param>
		private void Release(IDownloader downloader) {
			downloader.DownloadStarted -= new DownloadTaskStartedEventHandler( OnDownloadStarted );
			downloader.DownloadProgress -= new DownloadTaskProgressEventHandler( OnDownloadProgress );
			downloader.DownloadCompleted -= new DownloadTaskCompletedEventHandler( OnDownloadCompleted );
			downloader.DownloadError -= new DownloadTaskErrorEventHandler( OnDownloadError );
			
			IDisposable disposable = downloader as IDisposable;
			if(disposable != null){
				disposable.Dispose(); 
			}
		}

		#endregion
	}


	#region IDownloader
	/// <summary>
	/// Defines the contract that all downloaders must implement
	/// to be used as an updater downloader.
	/// </summary>
	public interface IDownloader  {
		/// <summary>
		/// Performs the synchronous download of the files specified in the manifest.
		/// </summary>
		/// <param name="task">The associated <see cref="DownloadTask"/> that holds a reference to the manifest to process</param>
		/// <param name="maxWaitTime">A time span indicating the maximum period of time to wait for a download. 
		/// An exception must be thrown if this period is exeeded.</param>
		 void Download(DownloadTask task, TimeSpan maxWaitTime); 
		
		/// <summary>
		/// Initiates the asynchronous download of the files specified in the manifest.
		/// </summary>
		/// <param name="task">The associated <see cref="DownloadTask"/> that holds a reference to the manifest to process.</param>
		void BeginDownload(DownloadTask task);

		/// <summary>
		/// Terminates or cancels an unfinished asynchronous download.
		/// </summary>
		/// <param name="task">The associated <see cref="DownloadTask"/> that holds a reference to the manifest to process</param>
		/// <returns>Returns true if the task was canceled.</returns>
		bool CancelDownload(DownloadTask task);

		/// <summary>
		/// Notifies about the download progress for the update.
		/// </summary>
		event DownloadTaskProgressEventHandler DownloadProgress;
		
		/// <summary>
		/// Notifies that the downloading for an DownloadTask has started.
		/// </summary>
		event DownloadTaskStartedEventHandler DownloadStarted;

		/// <summary>
		/// Notifies that the downloading for an DownloadTask has finished.
		/// </summary>
		event DownloadTaskCompletedEventHandler DownloadCompleted;

		/// <summary>
		/// Notifies that an error ocurred while downloading the files for an DownloadTask.
		/// </summary>
		event DownloadTaskErrorEventHandler DownloadError;

	}

	#endregion

	/// <summary>
	/// This is a generic exception thrown by the updater.
	/// </summary>
	[Serializable]
	public class DownloaderException : Exception {
		#region Constructor

		/// <summary>
		/// Default constructor.
		/// </summary>
		public DownloaderException() : base() {
		}

		/// <summary>
		/// Creates an DownloaderException with a specified message.
		/// </summary>
		/// <param name="message">The exception message string.</param>
		public DownloaderException( string message ) : base( message ) {
		}

		/// <summary>
		/// Creates an DownloaderException with a specified message and an inner exception.
		/// </summary>
		/// <param name="message">The exception message string.</param>
		/// <param name="innerException">The inner exception detected.</param>
		public DownloaderException( string message, Exception innerException ) : base( message, innerException ) {
		}

		/// <summary>
		/// Serialization constructor used for the serialization of the exception.
		/// </summary>
		/// <param name="info">The serialization information.</param>
		/// <param name="context">The serialization context.</param>
		[SecurityPermissionAttribute(SecurityAction.Demand, SerializationFormatter=true)]
		protected DownloaderException(SerializationInfo info, StreamingContext context) : base( info, context ) {
		}

		#endregion
	}

}

#region CVS Version Log
/*
 * $Log: BackgroundDownloadManager.cs,v $
 * Revision 1.16  2007/07/21 12:26:21  t_rendelmann
 * added support for "portable Bandit" version
 *
 * Revision 1.15  2007/06/14 00:59:40  carnage4life
 * We now delete BITS tasks once an enclosure has been downloaded or an error occured on download
 *
 * Revision 1.14  2007/02/17 14:45:52  t_rendelmann
 * switched: Resource.Manager indexer usage to strongly typed resources (StringResourceTool)
 *
 * Revision 1.13  2007/02/11 15:58:53  carnage4life
 * 1.) Added proper handling for when a podcast download exceeds the size limit on the podcast folder
 *
 * Revision 1.12  2006/12/24 15:18:54  carnage4life
 * Added support for erroring when enclosure cache limit is reached
 *
 * Revision 1.11  2006/12/19 17:00:39  t_rendelmann
 * added: CVS log sections
 *
 */
#endregion
