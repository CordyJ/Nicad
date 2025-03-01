#region CVS Version Header
/*
 * $Id: ToolbarHelper.cs,v 1.16 2007/03/05 16:31:47 t_rendelmann Exp $
 * Last modified by $Author: t_rendelmann $
 * Last modified at $Date: 2007/03/05 16:31:47 $
 * $Revision: 1.16 $
 */
#endregion

using System;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Windows.Forms;
using Infragistics.Win.UltraWinToolbars;
using NewsComponents.Utils;
using RssBandit.Resources;
using RssBandit.Utility.Keyboard;
using RssBandit.WebSearch;
using RssBandit.WinGui.Interfaces;
using RssBandit.WinGui.Tools;
using RssBandit.WinGui.Utility;
using Logger = RssBandit.Common.Logging;

namespace RssBandit.WinGui.Forms.ControlHelpers
{
	/// <summary>
	/// Just a helper class to create IG Toolbars.
	/// </summary>
	internal class ToolbarHelper
	{
		private UltraToolbarsManager manager;
		private WinGuiMain main;
		private RssBanditApplication owner;
		private ShortcutHandler shortcutHandler;
		private static readonly log4net.ILog _log = Logger.Log.GetLogger(typeof(RssBanditApplication));
		
		public ToolbarHelper(UltraToolbarsManager manager) {
			this.manager = manager;
		}

		public void CreateToolbars(WinGuiMain main, RssBanditApplication owner, ShortcutHandler shortcutHandler) {
			
			this.main = main;
			this.owner = owner;
			this.shortcutHandler = shortcutHandler;
			
			UltraToolbar webBrowser = new UltraToolbar(Resource.Toolbar.WebTools);
			UltraToolbar mainMenu = new UltraToolbar(Resource.Toolbar.MenuBar);
			UltraToolbar mainTools = new UltraToolbar(Resource.Toolbar.MainTools);
			UltraToolbar searchTools = new UltraToolbar(Resource.Toolbar.SearchTools);
			
			webBrowser.DockedColumn = 0;
			webBrowser.DockedRow = 2;
			webBrowser.FloatingSize = new System.Drawing.Size(100, 20);
			webBrowser.Text = SR.MainForm_MainWebToolbarCaption;
			
			mainMenu.DockedColumn = 0;
			mainMenu.DockedRow = 0;
			mainMenu.IsMainMenuBar = true;
			mainMenu.Text = SR.MainForm_MainMenuToolbarCaption;
			
			mainTools.DockedColumn = 0;
			mainTools.DockedRow = 1;
			mainTools.Text = SR.MainForm_MainAppToolbarCaption;
			
			searchTools.DockedColumn = 1;
			searchTools.DockedRow = 2;
			searchTools.Text = SR.MainForm_MainSearchToolbarCaption;
			
			this.manager.Toolbars.AddRange(
				new UltraToolbar[] {webBrowser, mainMenu, mainTools, searchTools});
			
			InitMenuBar();
			
			CreateMainToolbar(this.manager.Toolbars[Resource.Toolbar.MainTools]);
			CreateBrowserToolbar(this.manager.Toolbars[Resource.Toolbar.WebTools]);
			CreateSearchToolbar(this.manager.Toolbars[Resource.Toolbar.SearchTools]);
			
			this.main = null;
			this.owner = null;
			this.shortcutHandler = null;
		}

		public void CreateToolbars(NewsgroupsConfiguration main) {
			
			UltraToolbar mainTools = new UltraToolbar(Resource.Toolbar.MainTools);
			
			mainTools.DockedColumn = 0;
			mainTools.DockedRow = 0;
			mainTools.Text = SR.NewsGroupConfiguration_MainToolbarCaption;
				
			this.manager.Toolbars.AddRange(
				new UltraToolbar[] {  mainTools });
			
			Infragistics.Win.Appearance a = null;
			
			ButtonTool newIdentity = new ButtonTool("toolNewIndentity");
			newIdentity.SharedProps.Caption = SR.NewsGroupConfiguration_NewIdentityToolCaption;
			newIdentity.SharedProps.StatusText = newIdentity.SharedProps.ToolTipText = SR.NewsGroupConfiguration_NewIdentityToolDesc;
			a = new Infragistics.Win.Appearance();
			a.Image = Resource.LoadBitmap("Resources.add.user.16.png");
			newIdentity.SharedProps.AppearancesSmall.Appearance = a;
			newIdentity.SharedProps.DisplayStyle = ToolDisplayStyle.ImageAndText;
			
			ButtonTool newServer = new ButtonTool("toolNewNntpServer");
			newServer.SharedProps.Caption = SR.NewsGroupConfiguration_NewNewsServerToolCaption;
			newServer.SharedProps.StatusText = newServer.SharedProps.ToolTipText = SR.NewsGroupConfiguration_NewNewsServerToolDesc;
			a = new Infragistics.Win.Appearance();
			a.Image = Resource.LoadBitmap("Resources.add.newsserver.16.png");
			newServer.SharedProps.AppearancesSmall.Appearance = a;
			newServer.SharedProps.DisplayStyle = ToolDisplayStyle.ImageAndText;
			
			ButtonTool deleteItem = new ButtonTool("toolDelete");
			deleteItem.SharedProps.Caption = SR.NewsGroupConfiguration_DeleteToolCaption;
			deleteItem.SharedProps.StatusText = deleteItem.SharedProps.ToolTipText = SR.NewsGroupConfiguration_DeleteToolDesc;
			a = new Infragistics.Win.Appearance();
			a.Image = Resource.LoadBitmap("Resources.delete.16.png");
			deleteItem.SharedProps.AppearancesSmall.Appearance = a;
			deleteItem.SharedProps.DisplayStyle = ToolDisplayStyle.ImageAndText;
			
			// must be added to the toolbar first:
			this.manager.Tools.AddRange(new ToolBase[] {newIdentity, newServer, deleteItem});

			mainTools.Tools.AddRange(new ToolBase[]{newIdentity, newServer, deleteItem});
			foreach (ToolBase tool in mainTools.Tools) {
				tool.SharedProps.Category = SR.MainForm_ToolCategoryTools;
			}
			
			// now we can set instance properties:
			ToolBase t = mainTools.Tools["toolDelete"];
			t.InstanceProps.IsFirstInGroup = true;
		}
		
		/// <summary>
		/// Sets the toolbar visible state.
		/// </summary>
		/// <param name="toolbarId">The toolbar id.</param>
		/// <param name="visible">if set to <c>true</c> [visible].</param>
		public void SetToolbarVisible(string toolbarId, bool visible) {
			this.manager.Toolbars[toolbarId].Visible = visible;
		}
		/// <summary>
		/// Determines whether [is toolbar visible] [the specified toolbar id].
		/// </summary>
		/// <param name="toolbarId">The toolbar id.</param>
		/// <returns>
		/// 	<c>true</c> if [is toolbar visible] [the specified toolbar id]; otherwise, <c>false</c>.
		/// </returns>
		public bool IsToolbarVisible(string toolbarId) {
			return this.manager.Toolbars[toolbarId].Visible;
		}
		
		#region Menu Bar

		private void InitMenuBar() {
			AppPopupMenuCommand fileMenu = new AppPopupMenuCommand(
				"mnuFile", owner.Mediator, null, 
				SR.MainMenuFileCaption, SR.MainMenuFileDesc);
			fileMenu.SharedProps.ShowInCustomizer = false;
			
			AppPopupMenuCommand editMenu = new AppPopupMenuCommand(
				"mnuEdit", owner.Mediator, null, 
				SR.MainMenuEditCaption, SR.MainMenuEditDesc);
			editMenu.SharedProps.ShowInCustomizer = false;
			
			AppPopupMenuCommand viewMenu = new AppPopupMenuCommand(
				"mnuView", owner.Mediator, null, 
				SR.MainMenuViewCaption, SR.MainMenuViewDesc);
			viewMenu.SharedProps.ShowInCustomizer = false;
			
			AppPopupMenuCommand toolsMenu = new AppPopupMenuCommand(
				"mnuTools", owner.Mediator, null, 
				SR.MainMenuToolsCaption, SR.MainMenuToolsDesc);
			toolsMenu.SharedProps.ShowInCustomizer = false;
			
			AppPopupMenuCommand helpMenu = new AppPopupMenuCommand(
				"mnuHelp", owner.Mediator, null, 
				SR.MainMenuHelpCaption, SR.MainMenuHelpDesc);
			helpMenu.SharedProps.ShowInCustomizer = false;

			// must be added to the toolbar first:
			this.manager.Tools.AddRange(new ToolBase[] {fileMenu, editMenu, viewMenu, toolsMenu, helpMenu});
			
			// now add instances to:
			UltraToolbar menu = this.manager.Toolbars["tbMainMenu"];
			menu.Tools.AddRange(new ToolBase[]{fileMenu, editMenu, viewMenu, toolsMenu, helpMenu});
			foreach (ToolBase tool in menu.Tools) {
				tool.SharedProps.Category = SR.MainForm_ToolCategoryMenu;
			}

			CreateFileMenu(fileMenu);
			CreateEditMenu(editMenu);
			CreateViewMenu(viewMenu);
			CreateToolsMenu(toolsMenu);
			CreateHelpMenu(helpMenu);
		}
		
		private void CreateFileMenu(AppPopupMenuCommand mc) {
			// Create menu commands
			AppButtonToolCommand newSubscription = new AppButtonToolCommand(
				"cmdNewSubscription", owner.Mediator, new ExecuteCommandHandler(owner.CmdNewSubscription),
				SR.MenuNewSubscriptionByWizardCaption, SR.MenuNewSubscriptionByWizardDesc, 
				Resource.ToolItemImage.NewSubscription, shortcutHandler);

			AppButtonToolCommand importFeeds = new AppButtonToolCommand(
				"cmdImportFeeds", owner.Mediator, new ExecuteCommandHandler(owner.CmdImportFeeds),
				SR.MenuImportFeedsCaption, SR.MenuImportFeedsDesc, shortcutHandler);

			AppButtonToolCommand exportFeeds = new AppButtonToolCommand(
				"cmdExportFeeds", owner.Mediator, new ExecuteCommandHandler(owner.CmdExportFeeds),
				SR.MenuExportFeedsCaption, SR.MenuExportFeedsDesc, shortcutHandler);

			AppButtonToolCommand appExit = new AppButtonToolCommand(
				"cmdCloseExit", owner.Mediator, new ExecuteCommandHandler(owner.CmdExitApp),
				SR.MenuAppCloseExitCaption, SR.MenuAppCloseExitDesc, shortcutHandler);

			AppStateButtonToolCommand toggleOffline = new AppStateButtonToolCommand(
				"cmdToggleOfflineMode", owner.Mediator, new ExecuteCommandHandler(owner.CmdToggleInternetConnectionMode),
				SR.MenuAppInternetConnectionModeCaption, SR.MenuAppInternetConnectionModeDesc, shortcutHandler);

			// must be added to the toolbar first:
			this.manager.Tools.AddRange(new ToolBase[] {newSubscription,importFeeds,exportFeeds,toggleOffline,appExit});
			
			mc.Tools.AddRange(new ToolBase[]{newSubscription,importFeeds,exportFeeds,toggleOffline,appExit});
			foreach (ToolBase tool in mc.Tools) {
				tool.SharedProps.Category = SR.MainForm_ToolCategoryFile;
			}
			
			// now we can set instance properties:
			ToolBase t = mc.Tools["cmdImportFeeds"];
			t.InstanceProps.IsFirstInGroup = true;
			t = mc.Tools["cmdToggleOfflineMode"];
			t.InstanceProps.IsFirstInGroup = true;
			t = mc.Tools["cmdCloseExit"];
			t.InstanceProps.IsFirstInGroup = true;

		}
		
		private void CreateEditMenu(AppPopupMenuCommand mc) {
			            
			AppButtonToolCommand selectAll = new AppButtonToolCommand(
				"cmdSelectAllFeedItems", owner.Mediator, new ExecuteCommandHandler(main.CmdSelectAllNewsItems), 
				SR.MenuListViewSelectAllCaption, SR.MenuListViewSelectAllDesc, shortcutHandler);
			
			// must be added to the toolbar first:
			this.manager.Tools.AddRange(new ToolBase[] {selectAll});
			
			mc.Tools.AddRange(new ToolBase[]{selectAll});
			foreach (ToolBase tool in mc.Tools) {
				tool.SharedProps.Category = SR.MainForm_ToolCategoryEdit;
			}
		}

		private void CreateViewMenu(AppPopupMenuCommand mc) {
			
			AppStateButtonToolCommand toogleTreeViewState = new AppStateButtonToolCommand(
				"cmdToggleTreeViewState", owner.Mediator, new ExecuteCommandHandler(main.CmdDockShowSubscriptions),
				SR.MenuToggleTreeViewStateCaption, SR.MenuToggleTreeViewStateDesc, 
				Resource.ToolItemImage.ToggleSubscriptions, shortcutHandler);

			AppStateButtonToolCommand toggleRssSearchViewState = new AppStateButtonToolCommand(
				"cmdToggleRssSearchTabState", owner.Mediator, new ExecuteCommandHandler(main.CmdDockShowRssSearch),
				SR.MenuToggleRssSearchTabStateCaption, SR.MenuToggleRssSearchTabStateDesc, 
				Resource.ToolItemImage.ToggleRssSearch, shortcutHandler);

			AppPopupMenuCommand toolbarsDropDownMenu = new AppPopupMenuCommand(
				"mnuViewToolbars", owner.Mediator, null, 
				SR.MenuViewToolbarsCaption, SR.MenuViewToolbarsDesc);
			
			// subMenus:			
			AppStateButtonToolCommand subTbMain = new AppStateButtonToolCommand(
				"cmdToggleMainTBViewState", owner.Mediator, new ExecuteCommandHandler(main.CmdToggleMainTBViewState),
				SR.MenuViewToolbarMainCaption, SR.MenuViewToolbarMainDesc, shortcutHandler);
			subTbMain.Checked = true;	// default
			subTbMain.SharedProps.ShowInCustomizer = false;

			AppStateButtonToolCommand subTbWeb = new AppStateButtonToolCommand(
				"cmdToggleWebTBViewState", owner.Mediator, new ExecuteCommandHandler(main.CmdToggleWebTBViewState),
				SR.MenuViewToolbarWebNavigationCaption, SR.MenuViewToolbarWebNavigationDesc, shortcutHandler);
			subTbWeb.Checked = true;	// default
			subTbWeb.SharedProps.ShowInCustomizer = false;

			AppStateButtonToolCommand subTbWebSearch = new AppStateButtonToolCommand(
				"cmdToggleWebSearchTBViewState", owner.Mediator, new ExecuteCommandHandler(main.CmdToggleWebSearchTBViewState),
				SR.MenuViewToolbarWebSearchCaption, SR.MenuViewToolbarWebSearchDesc, shortcutHandler);
			subTbWebSearch.Checked = true;	// default
			subTbWebSearch.SharedProps.ShowInCustomizer = false;

			// must be added to the toolbar first:
			this.manager.Tools.AddRange(new ToolBase[] {subTbMain, subTbWeb, subTbWebSearch});
			
			toolbarsDropDownMenu.Tools.AddRange(new ToolBase[]{subTbMain, subTbWeb, subTbWebSearch});
			foreach (ToolBase tool in toolbarsDropDownMenu.Tools) {
				tool.SharedProps.Category = SR.MainForm_ToolCategoryView;
			}

			AppPopupMenuCommand columnChooserDropDownMenu = new AppPopupMenuCommand(
				"cmdColumnChooserMain", owner.Mediator, null,
				SR.MenuColumnChooserCaption, SR.MenuColumnChooserDesc);

			foreach (string colID in Enum.GetNames(typeof(NewsItemSortField))) {

				AppStateButtonToolCommand subL4_subColumn = new AppStateButtonToolCommand(
					"cmdListviewColumn." + colID, owner.Mediator, new ExecuteCommandHandler(main.CmdToggleListviewColumn),
					SR.Keys.GetString("MenuColumnChooser" + colID +"Caption"), 
					SR.Keys.GetString("MenuColumnChooser" + colID +"Desc"), shortcutHandler);
				
				subL4_subColumn.SharedProps.Category = SR.MainForm_ToolCategoryView;
				// must be added to the toolbar first:
				this.manager.Tools.AddRange(new ToolBase[] {subL4_subColumn});
				columnChooserDropDownMenu.Tools.AddRange(new ToolBase[]{subL4_subColumn});
			}

			AppButtonToolCommand subL4_subUseCatLayout = new AppButtonToolCommand(
				"cmdColumnChooserUseCategoryLayoutGlobal", owner.Mediator, new ExecuteCommandHandler(main.CmdColumnChooserUseCategoryLayoutGlobal),
				SR.MenuColumnChooserUseCategoryLayoutGlobalCaption, SR.MenuColumnChooserUseCategoryLayoutGlobalDesc, shortcutHandler);
			
			AppButtonToolCommand subL4_subUseFeedLayout = new AppButtonToolCommand(
				"cmdColumnChooserUseFeedLayoutGlobal", owner.Mediator, new ExecuteCommandHandler(main.CmdColumnChooserUseFeedLayoutGlobal),
				SR.MenuColumnChooserUseFeedLayoutGlobalCaption, SR.MenuColumnChooserUseFeedLayoutGlobalDesc, shortcutHandler);

			AppButtonToolCommand subL4_subResetLayout = new AppButtonToolCommand(
				"cmdColumnChooserResetToDefault", owner.Mediator, new ExecuteCommandHandler(main.CmdColumnChooserResetToDefault),
				SR.MenuColumnChooserResetLayoutToDefaultCaption, SR.MenuColumnChooserResetLayoutToDefaultDesc, shortcutHandler);
			subL4_subResetLayout.Enabled = false;		// dynamically refreshed

			// must be added to the toolbar first:
			this.manager.Tools.AddRange(new ToolBase[] {subL4_subUseCatLayout, subL4_subUseFeedLayout, subL4_subResetLayout});
			columnChooserDropDownMenu.Tools.AddRange(new ToolBase[]{ subL4_subUseCatLayout, subL4_subUseFeedLayout, subL4_subResetLayout});
			foreach (ToolBase tool in columnChooserDropDownMenu.Tools) {
				tool.SharedProps.Category = SR.MainForm_ToolCategoryView;
			}
			
			AppStateButtonToolCommand outlookReadingViewState = new AppStateButtonToolCommand(
				"cmdViewOutlookReadingPane", owner.Mediator, new ExecuteCommandHandler(main.CmdViewOutlookReadingPane),
				SR.MenuViewOutlookReadingPane, SR.MenuViewOutlookReadingPane, shortcutHandler);

			AppPopupMenuCommand layoutPositionDropDownMenu = new AppPopupMenuCommand(
				"cmdFeedDetailLayoutPosition", owner.Mediator, null,
				SR.MenuFeedDetailLayoutCaption, SR.MenuFeedDetailLayoutDesc);
			
			// subMenu:			
			AppStateButtonToolCommand subSub1 = new AppStateButtonToolCommand(
				"cmdFeedDetailLayoutPosTop", owner.Mediator, new ExecuteCommandHandler(main.CmdFeedDetailLayoutPosTop),
				SR.MenuFeedDetailLayoutTopCaption, SR.MenuFeedDetailLayoutTopDesc,
				Resource.ToolItemImage.ItemDetailViewAtTop, shortcutHandler);
			
			subSub1.Checked = true;	// default

			AppStateButtonToolCommand subSub2 = new AppStateButtonToolCommand(
				"cmdFeedDetailLayoutPosLeft", owner.Mediator, new ExecuteCommandHandler(main.CmdFeedDetailLayoutPosLeft),
				SR.MenuFeedDetailLayoutLeftCaption, SR.MenuFeedDetailLayoutLeftDesc,
				Resource.ToolItemImage.ItemDetailViewAtLeft, shortcutHandler);

			AppStateButtonToolCommand subSub3 = new AppStateButtonToolCommand(
				"cmdFeedDetailLayoutPosRight", owner.Mediator, new ExecuteCommandHandler(main.CmdFeedDetailLayoutPosRight),
				SR.MenuFeedDetailLayoutRightCaption, SR.MenuFeedDetailLayoutRightDesc, 
				Resource.ToolItemImage.ItemDetailViewAtRight, shortcutHandler);

			AppStateButtonToolCommand subSub4 = new AppStateButtonToolCommand(
				"cmdFeedDetailLayoutPosBottom", owner.Mediator, new ExecuteCommandHandler(main.CmdFeedDetailLayoutPosBottom),
				SR.MenuFeedDetailLayoutBottomCaption, SR.MenuFeedDetailLayoutBottomDesc, 
				Resource.ToolItemImage.ItemDetailViewAtBottom, shortcutHandler);

			// must be added to the toolbar first:
			this.manager.Tools.AddRange(new ToolBase[] {subSub1, subSub2, subSub3, subSub4});
			layoutPositionDropDownMenu.Tools.AddRange(new ToolBase[]{subSub1, subSub2, subSub3, subSub4});
			foreach (ToolBase tool in layoutPositionDropDownMenu.Tools) {
				tool.SharedProps.Category = SR.MainForm_ToolCategoryView;
			}

			// must be added to the toolbar first:
			this.manager.Tools.AddRange(new ToolBase[] {toogleTreeViewState,toggleRssSearchViewState,toolbarsDropDownMenu,layoutPositionDropDownMenu,columnChooserDropDownMenu,outlookReadingViewState});
			mc.Tools.AddRange(new ToolBase[]{toogleTreeViewState,toggleRssSearchViewState,toolbarsDropDownMenu,layoutPositionDropDownMenu,columnChooserDropDownMenu,outlookReadingViewState});
			foreach (ToolBase tool in mc.Tools) {
				tool.SharedProps.Category = SR.MainForm_ToolCategoryView;
			}
			
			// now we can set instance properties:
			ToolBase t = mc.Tools["mnuViewToolbars"];
			t.InstanceProps.IsFirstInGroup = true;
			t = columnChooserDropDownMenu.Tools["cmdColumnChooserUseCategoryLayoutGlobal"];
			t.InstanceProps.IsFirstInGroup = true;
			t = columnChooserDropDownMenu.Tools["cmdColumnChooserResetToDefault"];
			t.InstanceProps.IsFirstInGroup = true;
			t = mc.Tools["cmdFeedDetailLayoutPosition"];
			t.InstanceProps.IsFirstInGroup = true;
			
		}
		
		private void CreateToolsMenu(AppPopupMenuCommand mc) {
			       
			AppButtonToolCommand style1 = new AppButtonToolCommand(
				"cmdRefreshFeeds", owner.Mediator, new ExecuteCommandHandler(owner.CmdRefreshFeeds), 
				SR.MenuUpdateAllFeedsCaption, SR.MenuUpdateAllFeedsDesc, 
				Resource.ToolItemImage.RefreshAll, shortcutHandler);

			AppButtonToolCommand style2 = new AppButtonToolCommand(
				"cmdOpenConfigIdentitiesDialog", owner.Mediator, new ExecuteCommandHandler(main.CmdOpenConfigIdentitiesDialog),
				SR.MenuOpenConfigIdentitiesDialogCaption, SR.MenuOpenConfigIdentitiesDialogdesc, shortcutHandler);
			
			AppButtonToolCommand style3 = new AppButtonToolCommand(
				"cmdOpenConfigNntpServerDialog", owner.Mediator, new ExecuteCommandHandler(main.CmdOpenConfigNntpServerDialog),
				SR.MenuOpenConfigNntpServerDialogCaption, SR.MenuOpenConfigNntpServerDialogDesc, 
				shortcutHandler);
			style3.Enabled = true;		

			AppButtonToolCommand style51 = new AppButtonToolCommand(
				"cmdFeedItemNewPost", owner.Mediator, new ExecuteCommandHandler(owner.CmdPostNewItem),
				SR.MenuPostNewFeedItemCaption, SR.MenuPostNewFeedItemDesc, 
				Resource.ToolItemImage.NewPost,shortcutHandler);
			style51.Enabled = false;		// dynamically enabled on runtime if feed is NNTP

			AppButtonToolCommand style52 = new AppButtonToolCommand(
				"cmdFeedItemPostReply", owner.Mediator, new ExecuteCommandHandler(owner.CmdPostReplyToItem),
				SR.MenuPostReplyFeedItemCaption, SR.MenuPostReplyFeedItemDesc,
				Resource.ToolItemImage.PostReply, shortcutHandler);
			style52.Enabled = false;		// dynamically enabled on runtime if feed supports commentAPI
			
			AppButtonToolCommand style6 = new AppButtonToolCommand(
				"cmdUploadFeeds", owner.Mediator, new ExecuteCommandHandler(owner.CmdUploadFeeds),
				SR.MenuUploadFeedsCaption, SR.MenuUploadFeedsDesc, shortcutHandler);

			AppButtonToolCommand style7 = new AppButtonToolCommand(
				"cmdDownloadFeeds", owner.Mediator, new ExecuteCommandHandler(owner.CmdDownloadFeeds),
				SR.MenuDownloadFeedsCaption, SR.MenuDownloadFeedsDesc, shortcutHandler);

			AppButtonToolCommand style8 = new AppButtonToolCommand(
				"cmdOpenManageAddInsDialog", owner.Mediator, new ExecuteCommandHandler(owner.CmdOpenManageAddInsDialog),
				SR.MenuOpenManageAddInsDialogCaption, SR.MenuOpenManageAddInsDialogDesc,
				shortcutHandler);

			AppButtonToolCommand style9 = new AppButtonToolCommand(
				"cmdShowMainAppOptions", owner.Mediator, new ExecuteCommandHandler(owner.CmdShowOptions),
				SR.MenuAppOptionsCaption, SR.MenuAppOptionsDesc, 
				Resource.ToolItemImage.OptionsDialog, shortcutHandler);

			// must be added to the toolbar first:
			this.manager.Tools.AddRange(new ToolBase[] {style1, style51, style52, style6,style7, style2, style3, style8, style9});
			
			mc.Tools.AddRange(new ToolBase[]{style1, style51, style52, style6,style7, style2, style3, style8, style9});
			foreach (ToolBase tool in mc.Tools) {
				tool.SharedProps.Category = SR.MainForm_ToolCategoryTools;
			}
			// now we can set instance properties:
			ToolBase t = mc.Tools["cmdOpenConfigIdentitiesDialog"];
			t.InstanceProps.IsFirstInGroup = true;
			t = mc.Tools["cmdFeedItemNewPost"];
			t.InstanceProps.IsFirstInGroup = true;
			t = mc.Tools["cmdUploadFeeds"];
			t.InstanceProps.IsFirstInGroup = true;
			t = mc.Tools["cmdOpenManageAddInsDialog"];
			t.InstanceProps.IsFirstInGroup = true;
			t = mc.Tools["cmdShowMainAppOptions"];
			t.InstanceProps.IsFirstInGroup = true;
		}

		
		private void CreateHelpMenu(AppPopupMenuCommand mc) {
			            
			AppButtonToolCommand styleHelpWebDoc = new AppButtonToolCommand(
				"cmdHelpWebDoc", owner.Mediator, new ExecuteCommandHandler(owner.CmdWebHelp),
				SR.MenuWebHelpCaption, SR.MenuWebHelpDesc, shortcutHandler); 

			AppButtonToolCommand style0 = new AppButtonToolCommand(
				"cmdWorkspaceNews", owner.Mediator, new ExecuteCommandHandler(owner.CmdWorkspaceNews),
				SR.MenuWorkspaceNewsCaption, SR.MenuWorkspaceNewsDesc, shortcutHandler); 

			AppButtonToolCommand style1 = new AppButtonToolCommand(
				"cmdReportBug", owner.Mediator, new ExecuteCommandHandler(owner.CmdReportAppBug),
				SR.MenuBugReportCaption, SR.MenuBugReportDesc, shortcutHandler); 


			AppButtonToolCommand style2 = new AppButtonToolCommand(
				"cmdAbout", owner.Mediator, new ExecuteCommandHandler(owner.CmdAboutApp) , 
				SR.MenuAboutCaption, SR.MenuAboutDesc, shortcutHandler);

			style2.SharedProps.AppearancesSmall.Appearance.Image = Resource.LoadBitmap("Resources.rssbandit.16.png");
			style2.SharedProps.AppearancesLarge.Appearance.Image = Resource.LoadBitmap("Resources.rssbandit.32.png");
			
			AppButtonToolCommand style3 = new AppButtonToolCommand(
				"cmdCheckForUpdates", owner.Mediator, new ExecuteCommandHandler(owner.CmdCheckForUpdates),
				SR.MenuCheckForUpdatesCaption, SR.MenuCheckForUpdatesDesc, shortcutHandler);

			AppButtonToolCommand style4 = new AppButtonToolCommand(
				"cmdWikiNews", owner.Mediator, new ExecuteCommandHandler(owner.CmdWikiNews),
				SR.MenuBanditWikiCaption, SR.MenuBanditWikiDesc, shortcutHandler);
			
			AppButtonToolCommand style5 = new AppButtonToolCommand(
				"cmdVisitForum",owner.Mediator, new ExecuteCommandHandler(owner.CmdVisitForum),
				SR.MenuBanditForumCaption, SR.MenuBanditForumDesc, shortcutHandler);

			AppButtonToolCommand style6 = new AppButtonToolCommand(
				"cmdDonateToProject",owner.Mediator, new ExecuteCommandHandler(owner.CmdDonateToProject),
				SR.MenuDonateToProjectCaption, SR.MenuDonateToProjectDesc, shortcutHandler);

			
			AppButtonToolCommand sendLogs = new AppButtonToolCommand(
				"cmdSendLogsByMail", owner.Mediator, new ExecuteCommandHandler(owner.CmdSendLogsByMail),
				SR.MenuSendLogsByMailCaption, SR.MenuSendLogsByMailDesc, shortcutHandler);

			sendLogs.Enabled = false;
			try {
				if (File.Exists(RssBanditApplication.GetLogFileName()))
					sendLogs.Enabled = true;
			} catch { /* all */ }
			
			// must be added to the toolbar first:
			this.manager.Tools.AddRange(new ToolBase[] {sendLogs, styleHelpWebDoc, style4, style5, style0,style1,style3,style6,style2});
			mc.Tools.AddRange(new ToolBase[]{sendLogs, styleHelpWebDoc, style4, style5, style0,style1,style3,style6,style2});
			
			foreach (ToolBase tool in mc.Tools) {
				tool.SharedProps.Category = SR.MainForm_ToolCategoryHelp;
			}
			
			// now we can set instance properties:
			ToolBase t = mc.Tools["cmdAbout"];
			t.InstanceProps.IsFirstInGroup = true;
			t = mc.Tools["cmdCheckForUpdates"];
			t.InstanceProps.IsFirstInGroup = true;
			t = mc.Tools["cmdWikiNews"];
			t.InstanceProps.IsFirstInGroup = true;
			
		}


		#endregion

		#region Main Tools
		private void CreateMainToolbar(UltraToolbar tb) {
			// the "New..." dropdown tool:
			AppPopupMenuCommand toolNew = new AppPopupMenuCommand(
				"mnuNewFeed", owner.Mediator, new ExecuteCommandHandler(owner.CmdNewFeed),
				SR.MenuNewCmdsCaption, SR.MenuNewCmdsDesc, Resource.ToolItemImage.NewSubscription);
			toolNew.DropDownArrowStyle = DropDownArrowStyle.Segmented;
			toolNew.SharedProps.DisplayStyle = ToolDisplayStyle.ImageAndText;
			
			// ... and it's items:
			AppButtonToolCommand subNewFeed = new AppButtonToolCommand(
				"cmdNewFeed", owner.Mediator, new ExecuteCommandHandler(owner.CmdNewFeed), 
				SR.MenuNewFeedCaption, SR.MenuNewFeedDesc, Resource.ToolItemImage.NewSubscription);

			AppButtonToolCommand subNewNntp = new AppButtonToolCommand(
				"cmdNewNntpFeed", owner.Mediator, new ExecuteCommandHandler(owner.CmdNewNntpFeed), 
				SR.MenuNewNntpFeedCaption, SR.MenuNewNntpFeedDesc,
				Resource.ToolItemImage.NewNntpSubscription);

			AppButtonToolCommand subNewDiscovered = new AppButtonToolCommand(
				"cmdAutoDiscoverFeed", owner.Mediator, new ExecuteCommandHandler(owner.CmdAutoDiscoverFeed), 
				SR.MenuNewDiscoveredFeedCaption, SR.MenuNewDiscoveredFeedDesc,
				Resource.ToolItemImage.NewDiscoveredSubscription);
			
			this.manager.Tools.AddRange(new ToolBase[]{subNewFeed,subNewNntp,subNewDiscovered});
			toolNew.Tools.AddRange(new ToolBase[]{subNewFeed,subNewNntp,subNewDiscovered});
			foreach (ToolBase tool in toolNew.Tools) {
				tool.SharedProps.Category = SR.MainForm_ToolCategoryTools;
			}
			
			// more tool commands
			
			// re-use tool from menu:
			AppButtonToolCommand tool0 = (AppButtonToolCommand)this.manager.Tools["cmdRefreshFeeds"];
			tool0.SharedProps.DisplayStyle = ToolDisplayStyle.ImageAndText;
			
			AppButtonToolCommand tool1 = new AppButtonToolCommand(
				"cmdNextUnreadFeedItem", owner.Mediator, new ExecuteCommandHandler(owner.CmdNextUnreadFeedItem),
				SR.MenuNextUnreadItemCaption, SR.MenuNextUnreadItemDesc,
				Resource.ToolItemImage.NextUnreadItem);
			tool1.SharedProps.DisplayStyle = ToolDisplayStyle.ImageAndText;

			AppButtonToolCommand tool2 = new AppButtonToolCommand(
				"cmdCatchUpCurrentSelectedNode", owner.Mediator, new ExecuteCommandHandler(owner.CmdCatchUpCurrentSelectedNode),
				SR.MenuCatchUpSelectedNodeCaption, SR.MenuCatchUpSelectedNodeDesc,
				Resource.ToolItemImage.MarkAsRead);
			tool2.SharedProps.DisplayStyle = ToolDisplayStyle.ImageAndText;

			// re-use tool from menu:
			AppButtonToolCommand tool4 = (AppButtonToolCommand)this.manager.Tools["cmdFeedItemNewPost"];
			tool4.Enabled = false;

			// re-use tool from menu:
			AppButtonToolCommand tool5 = (AppButtonToolCommand)this.manager.Tools["cmdFeedItemPostReply"];
			tool5.SharedProps.DisplayStyle = ToolDisplayStyle.ImageAndText;
			tool5.Enabled = false;

			AppButtonToolCommand tool6 = new AppButtonToolCommand(
				"cmdNewRssSearch", owner.Mediator, new ExecuteCommandHandler(main.CmdNewRssSearch),
				SR.MenuNewRssSearchCaption, SR.MenuNewRssSearchDesc, 
				Resource.ToolItemImage.Search);

			AppPopupMenuCommand discoveredDropDown = new AppPopupMenuCommand(
				"cmdDiscoveredFeedsDropDown", owner.Mediator, null,
				SR.MenuAutodiscoveredFeedsDropdownCaption, SR.MenuAutodiscoveredFeedsDropdownDesc);
			
			Infragistics.Win.Appearance a = new Infragistics.Win.Appearance();
			a.Image = Resource.LoadBitmap("Resources.no.feed.discovered.16.png");
			discoveredDropDown.SharedProps.AppearancesSmall.Appearance = a;
			
			a = new Infragistics.Win.Appearance();
			a.Image = Resource.LoadBitmap("Resources.no.feed.discovered.32.png");
			discoveredDropDown.SharedProps.AppearancesLarge.Appearance = a;
			
			discoveredDropDown.SharedProps.DisplayStyle = ToolDisplayStyle.ImageOnlyOnToolbars;

			AppButtonToolCommand clearDiscoveredList = new AppButtonToolCommand(
				"cmdDiscoveredFeedsListClear", owner.Mediator, new ExecuteCommandHandler(owner.BackgroundDiscoverFeedsHandler.CmdClearFeedsList), 
				SR.MenuClearAutodiscoveredFeedsListCaption, SR.MenuClearAutodiscoveredFeedsListDesc);

			clearDiscoveredList.SharedProps.Category = SR.MainForm_ToolCategoryTools;
			
			// must be added to the toolbar first:
			this.manager.Tools.AddRange(new ToolBase[] {toolNew,tool1,tool2,tool6, discoveredDropDown, clearDiscoveredList});

			// set UI controls for discovered feeds handler:
			owner.BackgroundDiscoverFeedsHandler.SetControls(discoveredDropDown, clearDiscoveredList);
			
			tb.Tools.AddRange(new ToolBase[]{toolNew,tool0,tool1,tool2,tool4,tool5,tool6, discoveredDropDown});
			foreach (ToolBase tool in tb.Tools) {
				tool.SharedProps.Category = SR.MainForm_ToolCategoryTools;
			}
			
			// now we can set instance properties:
			ToolBase t = tb.Tools["cmdRefreshFeeds"];
			t.InstanceProps.IsFirstInGroup = true;
			t = tb.Tools["cmdNextUnreadFeedItem"];
			t.InstanceProps.IsFirstInGroup = true;
			t = tb.Tools["cmdFeedItemNewPost"];
			t.InstanceProps.IsFirstInGroup = true;
			t = tb.Tools["cmdNewRssSearch"];
			t.InstanceProps.IsFirstInGroup = true;
		}
		#endregion
		
		private void CreateBrowserToolbar(UltraToolbar tb) 
		{
			AppPopupMenuCommand tool0 = new AppPopupMenuCommand(
				"cmdBrowserGoBack", owner.Mediator, new ExecuteCommandHandler(owner.CmdBrowserGoBack),
				SR.MenuBrowserNavigateBackCaption, SR.MenuBrowserNavigateBackDesc,
				Resource.ToolItemImage.BrowserItemImageOffset + Resource.BrowserItemImage.GoBack);
			
			tool0.Enabled = false;
			tool0.DropDownArrowStyle = DropDownArrowStyle.Segmented;
			tool0.SharedProps.DisplayStyle = ToolDisplayStyle.ImageAndText;
			
			AppPopupMenuCommand tool1 = new AppPopupMenuCommand(
				"cmdBrowserGoForward", owner.Mediator, new ExecuteCommandHandler(owner.CmdBrowserGoForward),
				SR.MenuBrowserNavigateForwardCaption, SR.MenuBrowserNavigateForwardDesc,
				Resource.ToolItemImage.BrowserItemImageOffset + Resource.BrowserItemImage.GoForward);

			tool1.Enabled = false;
			tool1.DropDownArrowStyle = DropDownArrowStyle.Segmented;
			tool1.SharedProps.DisplayStyle = ToolDisplayStyle.ImageOnlyOnToolbars;

			AppButtonToolCommand tool2 = new AppButtonToolCommand(
				"cmdBrowserCancelNavigation", owner.Mediator, new ExecuteCommandHandler(owner.CmdBrowserCancelNavigation),
				SR.MenuBrowserNavigateCancelCaption, SR.MenuBrowserNavigateCancelDesc,
				Resource.ToolItemImage.BrowserItemImageOffset + Resource.BrowserItemImage.CancelNavigation);

			AppButtonToolCommand tool3 = new AppButtonToolCommand(
				"cmdBrowserRefresh", owner.Mediator, new ExecuteCommandHandler(owner.CmdBrowserRefresh),
				SR.MenuBrowserRefreshCaption, SR.MenuBrowserRefreshDesc, 
				Resource.ToolItemImage.BrowserItemImageOffset + Resource.BrowserItemImage.Refresh);
			
			ControlContainerTool urlDropdownContainerTool = new Infragistics.Win.UltraWinToolbars.ControlContainerTool("cmdUrlDropdownContainer");
			urlDropdownContainerTool.SharedProps.Caption = SR.MenuBrowserNavigateComboBoxCaption;
			urlDropdownContainerTool.SharedProps.StatusText = SR.MenuBrowserNavigateComboBoxDesc;
			urlDropdownContainerTool.SharedProps.MinWidth = 330;

#if USE_IG_URL_COMBOBOX		
			//TODO: future usage of the IG dropdown combo editor with item image support. 
			// But for now auto-completion is not yet full featured and 
			// we cannot apply/use the UrlExtender helper class:
			UltraComboEditor ultraComboEditor = new UltraComboEditor();
			ultraComboEditor.Name = "urlComboBox";
			// interconnect:
			urlDropdownContainerTool.Control = ultraComboEditor;
			main.Controls.Add(ultraComboEditor);
			
			ultraComboEditor.AllowDrop = true;
			ultraComboEditor.AutoComplete = true;
			ultraComboEditor.DisplayStyle = Infragistics.Win.EmbeddableElementDisplayStyle.Office2003;
			ultraComboEditor.DropDownButtonDisplayStyle = Infragistics.Win.ButtonDisplayStyle.OnMouseEnter;
			ultraComboEditor.ShowOverflowIndicator = true;
			ultraComboEditor.NullText = SR.MainForm_ToolUrlDropdownCueText;
			
			Infragistics.Win.Appearance a = new Infragistics.Win.Appearance();
			a.Image = Resource.LoadBitmap("Resources.Html.16.png");
			ultraComboEditor.Appearance = a;
			
			a = new Infragistics.Win.Appearance();
			a.Image = Resource.LoadBitmap("Resources.Html.16.png");
			ultraComboEditor.ItemAppearance = a;
			
			a = new Infragistics.Win.Appearance();
			a.ForeColor = System.Drawing.SystemColors.GrayText;
			ultraComboEditor.NullTextAppearance = a;
#else			
			ComboBox navigateComboBox = new ComboBox();
			navigateComboBox.Name = "tbUrlComboBox";
			main.toolTip.SetToolTip(navigateComboBox, SR.MenuBrowserNavigateComboBoxDesc);
			navigateComboBox.KeyDown += new KeyEventHandler(main.OnNavigateComboBoxKeyDown);
			navigateComboBox.KeyPress += new KeyPressEventHandler(main.OnAnyEnterKeyPress);
			navigateComboBox.DragOver += new DragEventHandler(main.OnNavigateComboBoxDragOver);
			navigateComboBox.DragDrop += new DragEventHandler(main.OnNavigateComboBoxDragDrop);

			navigateComboBox.AllowDrop = true;
			navigateComboBox.Width = 330;
			// interconnect:
			urlDropdownContainerTool.Control = navigateComboBox;
			main.UrlComboBox = navigateComboBox;
			main.Controls.Add(navigateComboBox);
			main.urlExtender.Add(navigateComboBox);
#endif

			AppButtonToolCommand tool5 = new AppButtonToolCommand(
				"cmdBrowserNavigate", owner.Mediator, new ExecuteCommandHandler(owner.CmdBrowserNavigate),
				SR.MenuBrowserDoNavigateCaption, SR.MenuBrowserDoNavigateDesc,
				Resource.ToolItemImage.BrowserItemImageOffset + Resource.BrowserItemImage.DoNavigate);

			tool5.SharedProps.DisplayStyle = ToolDisplayStyle.ImageAndText;
			
			AppButtonToolCommand tool6 = new AppButtonToolCommand(
				"cmdBrowserNewTab", owner.Mediator, new ExecuteCommandHandler(owner.CmdBrowserCreateNewTab),
				SR.MenuBrowserNewTabCaption, SR.MenuBrowserNewTabDesc, 
				Resource.ToolItemImage.BrowserItemImageOffset + Resource.BrowserItemImage.OpenNewTab);

			AppButtonToolCommand tool7 = new AppButtonToolCommand(
				"cmdBrowserNewExternalWindow", owner.Mediator, new ExecuteCommandHandler(main.CmdOpenLinkInExternalBrowser),
				SR.MenuBrowserNewExternalWindowCaption, SR.MenuBrowserNewExternalWindowDesc, 
				Resource.ToolItemImage.BrowserItemImageOffset + Resource.BrowserItemImage.OpenInExternalBrowser);

			// must be added to the toolbar first:
			this.manager.Tools.AddRange(new ToolBase[] {tool0,tool1,tool2,tool3,urlDropdownContainerTool,tool5,tool6,tool7});

			tb.Tools.AddRange(new ToolBase[]{tool0,tool1,tool2,tool3,urlDropdownContainerTool,tool5,tool6,tool7});
			foreach (ToolBase tool in tb.Tools) {
				tool.SharedProps.Category = SR.MainForm_ToolCategoryBrowse;
			}
			
			// set the both menu dropdowns within history manager:
			main.historyMenuManager.SetControls(tool0, tool1);

			// now we can set instance properties:
			ToolBase t = tb.Tools["cmdBrowserNewTab"];
			t.InstanceProps.IsFirstInGroup = true;
		}
		
		private void CreateSearchToolbar(UltraToolbar tb) {

			//_searchEngineImages.Images.Add(_browserImages.Images[Resource.BrowserItemImage.SearchWeb], Color.Magenta);

			ComboBox searchComboBox = new ComboBox();
			searchComboBox.Name = "tbSearchComboBox";
			main.toolTip.SetToolTip(searchComboBox, SR.MenuDoSearchComboBoxDesc);
			searchComboBox.KeyDown += new KeyEventHandler(main.OnSearchComboBoxKeyDown);
			searchComboBox.KeyPress += new KeyPressEventHandler(main.OnAnyEnterKeyPress);
			searchComboBox.DragOver += new DragEventHandler(main.OnSearchComboBoxDragOver);
			searchComboBox.DragDrop += new DragEventHandler(main.OnSearchComboBoxDragDrop);
			
			searchComboBox.AllowDrop = true;
			searchComboBox.Width = 150;
			searchComboBox.DropDownWidth = 350;

			//tb.Resize += new EventHandler(this.OnBarResize);
			//navigateComboBox.SelectedIndexChanged += new EventHandler(NavigationSelectedIndexChanged);

			ControlContainerTool searchDropdownContainerTool = new Infragistics.Win.UltraWinToolbars.ControlContainerTool("cmdSearchDropdownContainer");
			searchDropdownContainerTool.SharedProps.Caption = SR.MenuDoSearchComboBoxCaption;
			searchDropdownContainerTool.SharedProps.StatusText = SR.MenuDoSearchComboBoxDesc;
			searchDropdownContainerTool.SharedProps.MinWidth = 150;

			// interconnect:
			searchDropdownContainerTool.Control = searchComboBox;
			main.SearchComboBox = searchComboBox;
			main.Controls.Add(searchComboBox);
			
			AppPopupMenuCommand tool2 = new AppPopupMenuCommand(
				"cmdSearchGo", owner.Mediator, new ExecuteCommandHandler(main.CmdSearchGo),
				SR.MenuDoSearchWebCaption, SR.MenuDoSearchWebDesc, 
				Resource.ToolItemImage.BrowserItemImageOffset + Resource.BrowserItemImage.SearchWeb);
			
			tool2.DropDownArrowStyle = DropDownArrowStyle.Segmented;

			//_searchesGoCommand = tool2;	// need the reference later to build the search dropdown

			// must be added to the toolbar first:
			this.manager.Tools.AddRange(new ToolBase[] {searchDropdownContainerTool, tool2});
			
			tb.Tools.AddRange(new ToolBase[]{searchDropdownContainerTool, tool2});
			foreach (ToolBase tool in tb.Tools) {
				tool.SharedProps.Category = SR.MainForm_ToolCategorySearch;
			}
			//tb.StretchItem = searchComboBox;
			//tb.Stretch = true;
		}
		
		public void BuildSearchMenuDropdown(IList searchEngines, CommandMediator mediator, ExecuteCommandHandler executor) 
		{
			AppPopupMenuCommand parent = (AppPopupMenuCommand)manager.Toolbars[Resource.Toolbar.SearchTools].Tools["cmdSearchGo"];
			parent.Tools.Clear();
			parent.Enabled = false;
			foreach (SearchEngine engine in searchEngines) {
				AppButtonToolCommand item = null;
				string engineKey = "cmdExecuteSearchEngine" + engine.Title;
				if (manager.Tools.Exists(engineKey)) {
					item = (AppButtonToolCommand)manager.Tools[engineKey];
				} else {
					item = new AppButtonToolCommand(
						engineKey, mediator, executor, 
						engine.Title, engine.Description);
					manager.Tools.Add(item);
				}
				
				if (item.Mediator == null) {
					item.Mediator = mediator;
					item.OnExecute += executor;
					mediator.RegisterCommand(engineKey, item);
				} else
					if (item.Mediator != mediator) {
					mediator.ReRegisterCommand(item);
				}
				
				item.SharedProps.ShowInCustomizer = false;
				item.Tag = engine;

				Infragistics.Win.Appearance a = new Infragistics.Win.Appearance();
				item.SharedProps.AppearancesSmall.Appearance = a;
			
				if (engine.ImageName != null && engine.ImageName.Trim().Length > 0) {
					string p = Path.Combine(RssBanditApplication.GetSearchesPath(), engine.ImageName);
					if (File.Exists(p)) {
						Icon ico = null;
						Image img = null;
						try {
							if (Path.GetExtension(p).ToLower().EndsWith("ico")) {
								ico = new Icon(p);
								img = ico.ToBitmap();
							} 
							else {
								img = Image.FromFile(p);
							}
						}
						catch (Exception e) {
							_log.Error("Exception reading bitmap or Icon for searchEngine '" + engine.Title + "'.", e);
						}
						if (img != null) {
							a.Image = img;
						}
					}
				}
				parent.Tools.Add(item);
			}
			
			if (parent.Tools.Count > 0)
				parent.Enabled = true;
		}
		
		#region RssBanditToolbarManager class (derived from UltraToolbarsManager) 
 
		/// <summary> 
		/// This class is a custom tool provider that derives from UltraToolbarsManager. 
		/// See the IG docs for more information on the options available when creating custom tool providers. 
		///  
		/// The RssBanditToolbarManager class performs 2 important functions: 
		///        1. Registering the custom tool types that it supports (see the private ToolFactory class�s constructor) 
		///            When registering a custom tool type, the tool provider: 
		///                a. supplies a reference to an IToolProvider implementation 
		///                b. supplies a Guid to identify the custom tool type.  This Guid will be contained in the 
		///                   customizer�s request to create an instance of the custom tool type. 
		///                c. supplies a descriptive string used in the NewTool dialog to identify the custom tool 
		///                d. supplies a string to be used as the basis for the custom tool instance�s key and caption 
		///             
		///        2. Contains a private ToolFactory class that implements the IToolProvider interface which allows the 
		///           Customizer to call back when it needs to create an instance of a registered custom tool. 
		///            
		///    The RssBanditToolbarManager also overrides the virtual property ShowBuiltInToolTypesInCustomizer to  
		///    demonstrate how to prevent toolbars manager from displaying the built-in tools in the designtime 
		///    customizer. 
		/// </summary> 
		public class RssBanditToolbarManager : UltraToolbarsManager { 
			#region Member Variables 
 
			// A static reference to our private tool provider class.  An instance of this class is 
			// created in our static constructor. 
			private static RssBanditToolbarManager.ToolFactory toolFactory = null; 
			private static readonly log4net.ILog _log = Logger.Log.GetLogger(typeof(RssBanditToolbarManager));
			private bool isRegistered = false;
			#endregion Member Variables 
 
			#region Constructor 
 
			/// <summary> 
			/// Static constructor used to create a single instance of our private tool provider class. 
			/// </summary> 
			static RssBanditToolbarManager() { 
				RssBanditToolbarManager.toolFactory = new RssBanditToolbarManager.ToolFactory(); 
			} 
 
			/// <summary> 
			/// Standard constructor. 
			/// </summary> 
			public RssBanditToolbarManager():base() { 
				RssBanditToolbarManager.toolFactory.Register();
				this.isRegistered = true;
			} 
			
			/// <summary>
			/// Initializes a new instance of the <see cref="RssBanditToolbarManager"/> class.
			/// </summary>
			/// <param name="container">The container.</param>
			public RssBanditToolbarManager(IContainer container):base(container) {
				RssBanditToolbarManager.toolFactory.Register();
				this.isRegistered = true;
			}
			
			#endregion Constructor 
 
			#region Constants 
 
			internal static readonly Guid AppPopupMenuCommand_TOOLID = new Guid("B2C41B23-539C-402b-A991-0277C65AA91B"); 
			internal static readonly Guid AppButtonToolCommand_TOOLID = new Guid("BAD223CC-C7A0-4d28-9280-2A41049B2F82"); 
			internal static readonly Guid AppStateButtonToolCommand_TOOLID = new Guid("C3705644-795B-4ea7-9182-F4BC9DAD6AFE"); 
 
			#endregion Constants 
 
			#region Base Class Overrides

			#region Dispose
			/// <summary>
			/// Overriden. Invoked when the component has been disposed.
			/// </summary>
			protected override void Dispose(bool disposing) {
				if (this.isRegistered) {
					RssBanditToolbarManager.toolFactory.Unregister();
					this.isRegistered = false;
				}

				base.Dispose(disposing);
			}
			#endregion //Dispose

			
			#region ShowBuiltInToolTypesInCustomizer
	
			protected override bool ShowBuiltInToolTypesInCustomizer {
				get {
					// Comment-out the following line and uncomment the subsequent line to prevent the
					// toolbars manager's built-in tooltypes from being displayed in the runtime customizer.
					return true;
					//return false;
				}
			}

			#endregion ShowBuiltInToolTypesInCustomizer

			#endregion Base Class Overrides
 
			#region Private Class ToolFactory 
 
			/// <summary> 
			/// A private class that implements IToolProvider. 
			/// </summary> 
			private class ToolFactory : IToolProvider 
			{ 
				#region Member Variables

				private int counter = 0;

				#endregion //Member Variables
				
				#region Constructor 
				
 				#endregion Constructor 
 
				#region Register/Unregister
				internal void Register() {
					if (this.counter == 0)
						this.RegisterTools();

					this.counter++;
				}

				internal void Unregister() {
					this.counter--;

					System.Diagnostics.Debug.Assert(this.counter >= 0, "The Register/Unregister counter is out of sync!");

					if (this.counter == 0)
						this.UnregisterTools();
				}
				#endregion //Register/Unregister
				
				#region CreateToolInstance 
 
				/// <summary> 
				/// Creates and returns an instance of the tool identified by the specified GUID id. 
				/// </summary> 
				/// <param name="toolID">The Guid identifier specified for the tool when it was registered.</param> 
				/// <param name="key">The key assigned to the tool.</param> 
				/// <returns>A new instance of the specified tool.</returns> 
				ToolBase IToolProvider.CreateToolInstance(Guid toolID, string key) { 
					if (toolID == RssBanditToolbarManager.AppPopupMenuCommand_TOOLID) 
						return new AppPopupMenuCommand(key); 
 
					if (toolID == RssBanditToolbarManager.AppButtonToolCommand_TOOLID) 
						return new AppButtonToolCommand(key); 
 
					if (toolID == RssBanditToolbarManager.AppStateButtonToolCommand_TOOLID) 
						return new AppStateButtonToolCommand(key); 
 
					// The tool ID is unknown to us so return null. 
					return null; 
				} 
 
				#endregion CreateToolInstance 
				
				#region RegisterTools
				private void RegisterTools() {
					bool result; 
 
					// Register some custom tool types with UltraToolbarsManager. 
					try { 
						// AppPopupMenuCommand class. 
						result = UltraToolbarsManager.RegisterCustomToolType(this, RssBanditToolbarManager.AppPopupMenuCommand_TOOLID, "Application Popup Menu", "AppPopupMenuTool"); 
						if (result == false) 
							_log.Error("Error registering AppPopupMenuCommand class!"); 
 
						// AppButtonToolCommand class. 
						result = UltraToolbarsManager.RegisterCustomToolType(this, RssBanditToolbarManager.AppButtonToolCommand_TOOLID, "Application Button Tool", "AppButtonToolButton"); 
						if (result == false) 
							_log.Error("Error registering AppButtonToolCommand class!"); 

						// AppStateButtonToolCommand class. 
						result = UltraToolbarsManager.RegisterCustomToolType(this, RssBanditToolbarManager.AppStateButtonToolCommand_TOOLID, "Application State Button Tool", "AppStateButtonToolButton"); 
						if (result == false) 
							_log.Error("Error registering AppStateButtonToolCommand class!"); 
 
						
					} 
					catch { 
						_log.Error("Error registering custom tool classes!"); 
					} 
				}
				#endregion //RegisterTools

				#region UnregisterTools
				private void UnregisterTools() {
					UltraToolbarsManager.UnRegisterCustomToolType(RssBanditToolbarManager.AppPopupMenuCommand_TOOLID);
					UltraToolbarsManager.UnRegisterCustomToolType(RssBanditToolbarManager.AppButtonToolCommand_TOOLID);
					UltraToolbarsManager.UnRegisterCustomToolType(RssBanditToolbarManager.AppStateButtonToolCommand_TOOLID);
				}
				#endregion //UnregisterTools

			} 
 
			#endregion Private Class ToolFactory 
		} 
 
		#endregion ToolProviderAsManager class (derived from UltraToolbarsManager)
	}

}

#region CVS Version Log
/*
 * $Log: ToolbarHelper.cs,v $
 * Revision 1.16  2007/03/05 16:31:47  t_rendelmann
 * fixed: check for updates is always disabled
 *
 * Revision 1.15  2007/02/07 16:42:11  t_rendelmann
 * changed: started localization (de); removed unused assembly references
 *
 * Revision 1.14  2007/01/11 15:07:54  t_rendelmann
 * IG assemblies replaced by hotfix versions; migrated last Sandbar toolbar usage to IG ultratoolbar
 *
 * Revision 1.13  2006/12/16 20:30:37  t_rendelmann
 * fixed: too many items in toolbar customier (dynamic menu entries)
 *
 * Revision 1.12  2006/12/15 13:31:00  t_rendelmann
 * reworked to make dynamic menus work after toolbar gets loaded from .settings.xml
 *
 * Revision 1.10  2006/12/12 12:04:24  t_rendelmann
 * finished: all toolbar migrations; save/restore/customization works
 *
 * Revision 1.9  2006/12/10 20:54:14  t_rendelmann
 * cont.: search  toolbar migration finished - search dropdown now functional, commands dynamically build from search engines;
 * fixed: OutOfMemoryException on loading favicons
 *
 * Revision 1.8  2006/12/10 14:02:11  t_rendelmann
 * cont.: search toolbar migration started
 *
 * Revision 1.7  2006/12/10 12:34:09  t_rendelmann
 * cont.: web toolbar migration finished - url dropdown now functional
 *
 * Revision 1.6  2006/12/01 17:57:34  t_rendelmann
 * changed; next version with the new menubar,main toolbar web tools (functionality partially) migrated to IG - still work in progress
 *
 * Revision 1.5  2006/11/30 20:00:56  t_rendelmann
 * added new options to attachments tab
 *
 * Revision 1.4  2006/11/30 17:35:08  t_rendelmann
 * changed; next version with the new menubar,main toolbar web tools partially migrated to IG - still work in progress
 *
 * Revision 1.3  2006/11/30 17:12:54  t_rendelmann
 * changed; next version with the new menubar,main toolbar web tools partially migrated to IG - still work in progress
 *
 * Revision 1.2  2006/11/30 12:05:29  t_rendelmann
 * changed; next version with the new menubar and the main toolbar migrated to IG - still work in progress
 *
 * Revision 1.1  2006/11/28 18:08:41  t_rendelmann
 * changed; first version with the new menubar and the main toolbar migrated to IG - still work in progress
 *
 */
#endregion
