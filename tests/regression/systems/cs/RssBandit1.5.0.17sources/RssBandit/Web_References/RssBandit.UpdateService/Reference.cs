//------------------------------------------------------------------------------
// <autogenerated>
//     This code was generated by a tool.
//     Runtime Version: 1.1.4322.2032
//
//     Changes to this file may cause incorrect behavior and will be lost if 
//     the code is regenerated.
// </autogenerated>
//------------------------------------------------------------------------------

// 
// This source code was auto-generated by Microsoft.VSDesigner, Version 1.1.4322.2032.
// 
namespace RssBandit.UpdateService {
    using System.Diagnostics;
    using System.Xml.Serialization;
    using System;
    using System.Web.Services.Protocols;
    using System.ComponentModel;
    using System.Web.Services;
    
    
    /// <remarks/>
    [System.Diagnostics.DebuggerStepThroughAttribute()]
    [System.ComponentModel.DesignerCategoryAttribute("code")]
    [System.Web.Services.WebServiceBindingAttribute(Name="UpdateServiceSoap", Namespace="urn:schemas-rssbandit-org:rssbandit:update-services")]
    public class UpdateService : System.Web.Services.Protocols.SoapHttpClientProtocol {
        
        /// <remarks/>
        public UpdateService() {
            this.Url = "http://localhost/RssBandit.UpdateService/UpdateService.asmx";
        }
        
        /// <remarks/>
        [System.Web.Services.Protocols.SoapDocumentMethodAttribute("urn:schemas-rssbandit-org:rssbandit:update-services/DownloadLink", RequestNamespace="urn:schemas-rssbandit-org:rssbandit:update-services", ResponseNamespace="urn:schemas-rssbandit-org:rssbandit:update-services", Use=System.Web.Services.Description.SoapBindingUse.Literal, ParameterStyle=System.Web.Services.Protocols.SoapParameterStyle.Wrapped)]
        public string DownloadLink(string appID, string currentAppVersion, string appKey) {
            object[] results = this.Invoke("DownloadLink", new object[] {
                        appID,
                        currentAppVersion,
                        appKey});
            return ((string)(results[0]));
        }
        
        /// <remarks/>
        public System.IAsyncResult BeginDownloadLink(string appID, string currentAppVersion, string appKey, System.AsyncCallback callback, object asyncState) {
            return this.BeginInvoke("DownloadLink", new object[] {
                        appID,
                        currentAppVersion,
                        appKey}, callback, asyncState);
        }
        
        /// <remarks/>
        public string EndDownloadLink(System.IAsyncResult asyncResult) {
            object[] results = this.EndInvoke(asyncResult);
            return ((string)(results[0]));
        }
    }
}
