/**
 * @file led.ino
 * @brief This file contains the code for an ESP8266-based LED controller that can be configured via a web interface.
 * 
 * The code uses the ESPAsyncWebServer library to create a web server that serves a web page for configuring the device.
 * The configuration data is stored in EEPROM and loaded on startup. If no configuration is found, the device starts in 
 * access point mode and serves a web page for configuring the device.
 * 
 * The device has a rotary encoder with a push button for controlling the brightness of the LED. We also use the built-in
 * reset button (GPIO0) for resetting the WiFi configuration and the rotary encoder push button for toggling the LED on/off.
 * 
 * The code also uses the cAppConfig class to manage the configuration data stored in EEPROM.
 * 
 * @authors
 * duoquote
 * duoslow
 */

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);

const byte DNS_PORT = 53;
DNSServer dnsServer;

const uint8_t CONFIG_VERSION = 2;

const int LED_PIN = 4;  // GPIO4 (D2)
const int CLK = 14;     // GPIO14 (D5)
const int DT = 12;      // GPIO12 (D6)
const int SW = 13;      // GPIO13 (D7)
const int HW_RST = 0;   // GPIO0

int lastCLKState;
int currentStateCLK;
int rstCounter = 0;
unsigned long lastRSTPress = 0;
unsigned long lastRotaryUpdate = 0;
unsigned long lastRotaryPress = 0;
String ESP_SSID = "ESP-" + String(ESP.getChipId(), HEX);

const char *PARAM_INPUT = "value";

struct configData_t {
  uint8_t signature[2];
  uint8_t version;

  char ssid[32];
  char password[64];
  char token[64];

  uint16_t brightness;
};

// Config module is based on this blog post:
// https://arduinoplusplus.wordpress.com/2019/04/02/persisting-application-parameters-in-eeprom/
class cAppConfig {
private:
  const uint16_t SELECT_DELAY_DEFAULT = 1000;  // milliseconds
  const uint16_t LAST_SLOT_DEFAULT = 99;       // number
  const uint16_t EEPROM_ADDR = 0;
  const uint8_t EEPROM_SIG[2] = { 0xee, 0x11 };
  const uint8_t CONFIG_VERSION = 2;
  configData_t _D;

public:
  inline uint16_t getBrightness() {
    return (_D.brightness);
  };
  inline void setBrightness(uint16_t n) {
    _D.brightness = n;
  };
  inline char *getSSID() {
    return (_D.ssid);
  };
  inline void setSSID(char *n) {
    strncpy(_D.ssid, n, sizeof(_D.ssid));
  };
  inline char *getPassword() {
    return (_D.password);
  };
  inline void setPassword(char *n) {
    strncpy(_D.password, n, sizeof(_D.password));
  };
  inline char *getToken() {
    return (_D.token);
  };
  inline void setToken(char *n) {
    strncpy(_D.token, n, sizeof(_D.token));
  };

  void begin() {
    if (!configLoad()) {
      configDefault();
      configSave();
    }
  };

  void configDefault(void) {
    _D.signature[0] = EEPROM_SIG[0];
    _D.signature[1] = EEPROM_SIG[1];
    _D.version = CONFIG_VERSION;
    _D.brightness = 100;
  }

  bool configLoad(void) {
    EEPROM.get(EEPROM_ADDR, _D);
    if (_D.signature[0] != EEPROM_SIG[0] && _D.signature[1] != EEPROM_SIG[1])
      return (false);

    if (_D.version != CONFIG_VERSION) {
      configDefault();
      configSave();
    }

    _D.version = CONFIG_VERSION;

    return (true);
  }

  bool configSave(void) {
    EEPROM.put(EEPROM_ADDR, _D);
    EEPROM.commit();
    return (true);
  }
};

cAppConfig appConfig;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char setup_html[] PROGMEM = R"rawliteral(
<!doctype html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Edit Config</title><style>body,html{background-color:#222;color:#fff;height:100%}</style><script type="module" crossorigin>
(function(){const r=document.createElement("link").relList;if(r&&r.supports&&r.supports("modulepreload"))return;for(const o of document.querySelectorAll('link[rel="modulepreload"]'))t(o);new MutationObserver(o=>{for(const a of o)if(a.type==="childList")for(const s of a.addedNodes)s.tagName==="LINK"&&s.rel==="modulepreload"&&t(s)}).observe(document,{childList:!0,subtree:!0});function n(o){const a={};return o.integrity&&(a.integrity=o.integrity),o.referrerPolicy&&(a.referrerPolicy=o.referrerPolicy),o.crossOrigin==="use-credentials"?a.credentials="include":o.crossOrigin==="anonymous"?a.credentials="omit":a.credentials="same-origin",a}function t(o){if(o.ep)return;o.ep=!0;const a=n(o);fetch(o.href,a)}})();var me,h,ar,I,Re,or,xe,W={},ir=[],jr=/acit|ex(?:s|g|n|p|$)|rph|grid|ows|mnc|ntw|ine[ch]|zoo|^ord|itera/i,Ee=Array.isArray;function L(e,r){for(var n in r)e[n]=r[n];return e}function sr(e){var r=e.parentNode;r&&r.removeChild(e)}function zr(e,r,n){var t,o,a,s={};for(a in r)a=="key"?t=r[a]:a=="ref"?o=r[a]:s[a]=r[a];if(arguments.length>2&&(s.children=arguments.length>3?me.call(arguments,2):n),typeof e=="function"&&e.defaultProps!=null)for(a in e.defaultProps)s[a]===void 0&&(s[a]=e.defaultProps[a]);return te(e,s,t,o,null)}function te(e,r,n,t,o){var a={type:e,props:r,key:n,ref:t,__k:null,__:null,__b:0,__e:null,__d:void 0,__c:null,__h:null,constructor:void 0,__v:o??++ar};return o==null&&h.vnode!=null&&h.vnode(a),a}function he(e){return e.children}function ae(e,r){this.props=e,this.context=r}function B(e,r){if(r==null)return e.__?B(e.__,e.__.__k.indexOf(e)+1):null;for(var n;r<e.__k.length;r++)if((n=e.__k[r])!=null&&n.__e!=null)return n.__d||n.__e;return typeof e.type=="function"?B(e):null}function cr(e){var r,n;if((e=e.__)!=null&&e.__c!=null){for(e.__e=e.__c.base=null,r=0;r<e.__k.length;r++)if((n=e.__k[r])!=null&&n.__e!=null){e.__e=e.__c.base=n.__e;break}return cr(e)}}function Me(e){(!e.__d&&(e.__d=!0)&&I.push(e)&&!ue.__r++||Re!==h.debounceRendering)&&((Re=h.debounceRendering)||or)(ue)}function ue(){var e,r,n,t,o,a,s,c,l;for(I.sort(xe);e=I.shift();)e.__d&&(r=I.length,t=void 0,o=void 0,a=void 0,c=(s=(n=e).__v).__e,(l=n.__P)&&(t=[],o=[],(a=L({},s)).__v=s.__v+1,Ae(l,s,a,n.__n,l.ownerSVGElement!==void 0,s.__h!=null?[c]:null,t,c??B(s),s.__h,o),dr(t,s,o),s.__e!=c&&cr(s)),I.length>r&&I.sort(xe));ue.__r=0}function lr(e,r,n,t,o,a,s,c,l,d,u){var i,g,_,f,p,S,m,b,x,j=0,k=t&&t.__k||ir,A=k.length,v=A,q=r.length;for(n.__k=[],i=0;i<q;i++)(f=n.__k[i]=(f=r[i])==null||typeof f=="boolean"||typeof f=="function"?null:typeof f=="string"||typeof f=="number"||typeof f=="bigint"?te(null,f,null,null,f):Ee(f)?te(he,{children:f},null,null,null):f.__b>0?te(f.type,f.props,f.key,f.ref?f.ref:null,f.__v):f)!=null?(f.__=n,f.__b=n.__b+1,(b=Er(f,k,m=i+j,v))===-1?_=W:(_=k[b]||W,k[b]=void 0,v--),Ae(e,f,_,o,a,s,c,l,d,u),p=f.__e,(g=f.ref)&&_.ref!=g&&(_.ref&&Pe(_.ref,null,f),u.push(g,f.__c||p,f)),p!=null&&(S==null&&(S=p),(x=_===W||_.__v===null)?b==-1&&j--:b!==m&&(b===m+1?j++:b>m?v>q-m?j+=b-m:j--:j=b<m&&b==m-1?b-m:0),m=i+j,typeof f.type!="function"||b===m&&_.__k!==f.__k?typeof f.type=="function"||b===m&&!x?f.__d!==void 0?(l=f.__d,f.__d=void 0):l=p.nextSibling:l=fr(e,p,l):l=ur(f,l,e),typeof n.type=="function"&&(n.__d=l))):(_=k[i])&&_.key==null&&_.__e&&(_.__e==l&&(_.__=t,l=B(_)),Se(_,_,!1),k[i]=null);for(n.__e=S,i=A;i--;)k[i]!=null&&(typeof n.type=="function"&&k[i].__e!=null&&k[i].__e==n.__d&&(n.__d=k[i].__e.nextSibling),Se(k[i],k[i]))}function ur(e,r,n){for(var t,o=e.__k,a=0;o&&a<o.length;a++)(t=o[a])&&(t.__=e,r=typeof t.type=="function"?ur(t,r,n):fr(n,t.__e,r));return r}function fr(e,r,n){return n==null||n.parentNode!==e?e.insertBefore(r,null):r==n&&r.parentNode!=null||e.insertBefore(r,n),r.nextSibling}function Er(e,r,n,t){var o=e.key,a=e.type,s=n-1,c=n+1,l=r[n];if(l===null||l&&o==l.key&&a===l.type)return n;if(t>(l!=null?1:0))for(;s>=0||c<r.length;){if(s>=0){if((l=r[s])&&o==l.key&&a===l.type)return s;s--}if(c<r.length){if((l=r[c])&&o==l.key&&a===l.type)return c;c++}}return-1}function Ar(e,r,n,t,o){var a;for(a in n)a==="children"||a==="key"||a in r||fe(e,a,null,n[a],t);for(a in r)o&&typeof r[a]!="function"||a==="children"||a==="key"||a==="value"||a==="checked"||n[a]===r[a]||fe(e,a,r[a],n[a],t)}function Le(e,r,n){r[0]==="-"?e.setProperty(r,n??""):e[r]=n==null?"":typeof n!="number"||jr.test(r)?n:n+"px"}function fe(e,r,n,t,o){var a;e:if(r==="style")if(typeof n=="string")e.style.cssText=n;else{if(typeof t=="string"&&(e.style.cssText=t=""),t)for(r in t)n&&r in n||Le(e.style,r,"");if(n)for(r in n)t&&n[r]===t[r]||Le(e.style,r,n[r])}else if(r[0]==="o"&&r[1]==="n")a=r!==(r=r.replace(/(PointerCapture)$|Capture$/,"$1")),r=r.toLowerCase()in e?r.toLowerCase().slice(2):r.slice(2),e.l||(e.l={}),e.l[r+a]=n,n?t?n.u=t.u:(n.u=Date.now(),e.addEventListener(r,a?qe:Ie,a)):e.removeEventListener(r,a?qe:Ie,a);else if(r!=="dangerouslySetInnerHTML"){if(o)r=r.replace(/xlink(H|:h)/,"h").replace(/sName$/,"s");else if(r!=="width"&&r!=="height"&&r!=="href"&&r!=="list"&&r!=="form"&&r!=="tabIndex"&&r!=="download"&&r!=="rowSpan"&&r!=="colSpan"&&r!=="role"&&r in e)try{e[r]=n??"";break e}catch{}typeof n=="function"||(n==null||n===!1&&r[4]!=="-"?e.removeAttribute(r):e.setAttribute(r,n))}}function Ie(e){var r=this.l[e.type+!1];if(e.t){if(e.t<=r.u)return}else e.t=Date.now();return r(h.event?h.event(e):e)}function qe(e){return this.l[e.type+!0](h.event?h.event(e):e)}function Ae(e,r,n,t,o,a,s,c,l,d){var u,i,g,_,f,p,S,m,b,x,j,k,A,v,q,O=r.type;if(r.constructor!==void 0)return null;n.__h!=null&&(l=n.__h,c=r.__e=n.__e,r.__h=null,a=[c]),(u=h.__b)&&u(r);e:if(typeof O=="function")try{if(m=r.props,b=(u=O.contextType)&&t[u.__c],x=u?b?b.props.value:u.__:t,n.__c?S=(i=r.__c=n.__c).__=i.__E:("prototype"in O&&O.prototype.render?r.__c=i=new O(m,x):(r.__c=i=new ae(m,x),i.constructor=O,i.render=Nr),b&&b.sub(i),i.props=m,i.state||(i.state={}),i.context=x,i.__n=t,g=i.__d=!0,i.__h=[],i._sb=[]),i.__s==null&&(i.__s=i.state),O.getDerivedStateFromProps!=null&&(i.__s==i.state&&(i.__s=L({},i.__s)),L(i.__s,O.getDerivedStateFromProps(m,i.__s))),_=i.props,f=i.state,i.__v=r,g)O.getDerivedStateFromProps==null&&i.componentWillMount!=null&&i.componentWillMount(),i.componentDidMount!=null&&i.__h.push(i.componentDidMount);else{if(O.getDerivedStateFromProps==null&&m!==_&&i.componentWillReceiveProps!=null&&i.componentWillReceiveProps(m,x),!i.__e&&(i.shouldComponentUpdate!=null&&i.shouldComponentUpdate(m,i.__s,x)===!1||r.__v===n.__v)){for(r.__v!==n.__v&&(i.props=m,i.state=i.__s,i.__d=!1),r.__e=n.__e,r.__k=n.__k,r.__k.forEach(function(X){X&&(X.__=r)}),j=0;j<i._sb.length;j++)i.__h.push(i._sb[j]);i._sb=[],i.__h.length&&s.push(i);break e}i.componentWillUpdate!=null&&i.componentWillUpdate(m,i.__s,x),i.componentDidUpdate!=null&&i.__h.push(function(){i.componentDidUpdate(_,f,p)})}if(i.context=x,i.props=m,i.__P=e,i.__e=!1,k=h.__r,A=0,"prototype"in O&&O.prototype.render){for(i.state=i.__s,i.__d=!1,k&&k(r),u=i.render(i.props,i.state,i.context),v=0;v<i._sb.length;v++)i.__h.push(i._sb[v]);i._sb=[]}else do i.__d=!1,k&&k(r),u=i.render(i.props,i.state,i.context),i.state=i.__s;while(i.__d&&++A<25);i.state=i.__s,i.getChildContext!=null&&(t=L(L({},t),i.getChildContext())),g||i.getSnapshotBeforeUpdate==null||(p=i.getSnapshotBeforeUpdate(_,f)),lr(e,Ee(q=u!=null&&u.type===he&&u.key==null?u.props.children:u)?q:[q],r,n,t,o,a,s,c,l,d),i.base=r.__e,r.__h=null,i.__h.length&&s.push(i),S&&(i.__E=i.__=null)}catch(X){r.__v=null,(l||a!=null)&&(r.__e=c,r.__h=!!l,a[a.indexOf(c)]=null),h.__e(X,r,n)}else a==null&&r.__v===n.__v?(r.__k=n.__k,r.__e=n.__e):r.__e=Pr(n.__e,r,n,t,o,a,s,l,d);(u=h.diffed)&&u(r)}function dr(e,r,n){for(var t=0;t<n.length;t++)Pe(n[t],n[++t],n[++t]);h.__c&&h.__c(r,e),e.some(function(o){try{e=o.__h,o.__h=[],e.some(function(a){a.call(o)})}catch(a){h.__e(a,o.__v)}})}function Pr(e,r,n,t,o,a,s,c,l){var d,u,i,g=n.props,_=r.props,f=r.type,p=0;if(f==="svg"&&(o=!0),a!=null){for(;p<a.length;p++)if((d=a[p])&&"setAttribute"in d==!!f&&(f?d.localName===f:d.nodeType===3)){e=d,a[p]=null;break}}if(e==null){if(f===null)return document.createTextNode(_);e=o?document.createElementNS("http://www.w3.org/2000/svg",f):document.createElement(f,_.is&&_),a=null,c=!1}if(f===null)g===_||c&&e.data===_||(e.data=_);else{if(a=a&&me.call(e.childNodes),u=(g=n.props||W).dangerouslySetInnerHTML,i=_.dangerouslySetInnerHTML,!c){if(a!=null)for(g={},p=0;p<e.attributes.length;p++)g[e.attributes[p].name]=e.attributes[p].value;(i||u)&&(i&&(u&&i.__html==u.__html||i.__html===e.innerHTML)||(e.innerHTML=i&&i.__html||""))}if(Ar(e,_,g,o,c),i)r.__k=[];else if(lr(e,Ee(p=r.props.children)?p:[p],r,n,t,o&&f!=="foreignObject",a,s,a?a[0]:n.__k&&B(n,0),c,l),a!=null)for(p=a.length;p--;)a[p]!=null&&sr(a[p]);c||("value"in _&&(p=_.value)!==void 0&&(p!==e.value||f==="progress"&&!p||f==="option"&&p!==g.value)&&fe(e,"value",p,g.value,!1),"checked"in _&&(p=_.checked)!==void 0&&p!==e.checked&&fe(e,"checked",p,g.checked,!1))}return e}function Pe(e,r,n){try{typeof e=="function"?e(r):e.current=r}catch(t){h.__e(t,n)}}function Se(e,r,n){var t,o;if(h.unmount&&h.unmount(e),(t=e.ref)&&(t.current&&t.current!==e.__e||Pe(t,null,r)),(t=e.__c)!=null){if(t.componentWillUnmount)try{t.componentWillUnmount()}catch(a){h.__e(a,r)}t.base=t.__P=null,e.__c=void 0}if(t=e.__k)for(o=0;o<t.length;o++)t[o]&&Se(t[o],r,n||typeof e.type!="function");n||e.__e==null||sr(e.__e),e.__=e.__e=e.__d=void 0}function Nr(e,r,n){return this.constructor(e,n)}function Tr(e,r,n){var t,o,a,s;h.__&&h.__(e,r),o=(t=typeof n=="function")?null:n&&n.__k||r.__k,a=[],s=[],Ae(r,e=(!t&&n||r).__k=zr(he,null,[e]),o||W,W,r.ownerSVGElement!==void 0,!t&&n?[n]:o?null:r.firstChild?me.call(r.childNodes):null,a,!t&&n?n:o?o.__e:r.firstChild,t,s),dr(a,e,s)}me=ir.slice,h={__e:function(e,r,n,t){for(var o,a,s;r=r.__;)if((o=r.__c)&&!o.__)try{if((a=o.constructor)&&a.getDerivedStateFromError!=null&&(o.setState(a.getDerivedStateFromError(e)),s=o.__d),o.componentDidCatch!=null&&(o.componentDidCatch(e,t||{}),s=o.__d),s)return o.__E=o}catch(c){e=c}throw e}},ar=0,ae.prototype.setState=function(e,r){var n;n=this.__s!=null&&this.__s!==this.state?this.__s:this.__s=L({},this.state),typeof e=="function"&&(e=e(L({},n),this.props)),e&&L(n,e),e!=null&&this.__v&&(r&&this._sb.push(r),Me(this))},ae.prototype.forceUpdate=function(e){this.__v&&(this.__e=!0,e&&this.__h.push(e),Me(this))},ae.prototype.render=he,I=[],or=typeof Promise=="function"?Promise.prototype.then.bind(Promise.resolve()):setTimeout,xe=function(e,r){return e.__v.__b-r.__v.__b},ue.__r=0;var K,$,ye,De,$e=0,pr=[],oe=[],We=h.__b,Fe=h.__r,Ue=h.diffed,Ge=h.__c,Ve=h.unmount;function Ne(e,r){h.__h&&h.__h($,e,$e||r),$e=0;var n=$.__H||($.__H={__:[],__h:[]});return e>=n.__.length&&n.__.push({__V:oe}),n.__[e]}function ee(e){return $e=1,Or(mr,e)}function Or(e,r,n){var t=Ne(K++,2);if(t.t=e,!t.__c&&(t.__=[n?n(r):mr(void 0,r),function(c){var l=t.__N?t.__N[0]:t.__[0],d=t.t(l,c);l!==d&&(t.__N=[d,t.__[1]],t.__c.setState({}))}],t.__c=$,!$.u)){var o=function(c,l,d){if(!t.__c.__H)return!0;var u=t.__c.__H.__.filter(function(g){return g.__c});if(u.every(function(g){return!g.__N}))return!a||a.call(this,c,l,d);var i=!1;return u.forEach(function(g){if(g.__N){var _=g.__[0];g.__=g.__N,g.__N=void 0,_!==g.__[0]&&(i=!0)}}),!(!i&&t.__c.props===c)&&(!a||a.call(this,c,l,d))};$.u=!0;var a=$.shouldComponentUpdate,s=$.componentWillUpdate;$.componentWillUpdate=function(c,l,d){if(this.__e){var u=a;a=void 0,o(c,l,d),a=u}s&&s.call(this,c,l,d)},$.shouldComponentUpdate=o}return t.__N||t.__}function Hr(e,r){var n=Ne(K++,3);!h.__s&&_r(n.__H,r)&&(n.__=e,n.i=r,$.__H.__h.push(n))}function Rr(e,r){var n=Ne(K++,7);return _r(n.__H,r)?(n.__V=e(),n.i=r,n.__h=e,n.__V):n.__}function Mr(){for(var e;e=pr.shift();)if(e.__P&&e.__H)try{e.__H.__h.forEach(ie),e.__H.__h.forEach(Ce),e.__H.__h=[]}catch(r){e.__H.__h=[],h.__e(r,e.__v)}}h.__b=function(e){$=null,We&&We(e)},h.__r=function(e){Fe&&Fe(e),K=0;var r=($=e.__c).__H;r&&(ye===$?(r.__h=[],$.__h=[],r.__.forEach(function(n){n.__N&&(n.__=n.__N),n.__V=oe,n.__N=n.i=void 0})):(r.__h.forEach(ie),r.__h.forEach(Ce),r.__h=[],K=0)),ye=$},h.diffed=function(e){Ue&&Ue(e);var r=e.__c;r&&r.__H&&(r.__H.__h.length&&(pr.push(r)!==1&&De===h.requestAnimationFrame||((De=h.requestAnimationFrame)||Lr)(Mr)),r.__H.__.forEach(function(n){n.i&&(n.__H=n.i),n.__V!==oe&&(n.__=n.__V),n.i=void 0,n.__V=oe})),ye=$=null},h.__c=function(e,r){r.some(function(n){try{n.__h.forEach(ie),n.__h=n.__h.filter(function(t){return!t.__||Ce(t)})}catch(t){r.some(function(o){o.__h&&(o.__h=[])}),r=[],h.__e(t,n.__v)}}),Ge&&Ge(e,r)},h.unmount=function(e){Ve&&Ve(e);var r,n=e.__c;n&&n.__H&&(n.__H.__.forEach(function(t){try{ie(t)}catch(o){r=o}}),n.__H=void 0,r&&h.__e(r,n.__v))};var Be=typeof requestAnimationFrame=="function";function Lr(e){var r,n=function(){clearTimeout(t),Be&&cancelAnimationFrame(r),setTimeout(e)},t=setTimeout(n,100);Be&&(r=requestAnimationFrame(n))}function ie(e){var r=$,n=e.__c;typeof n=="function"&&(e.__c=void 0,n()),$=r}function Ce(e){var r=$;e.__c=e.__(),$=r}function _r(e,r){return!e||e.length!==r.length||r.some(function(n,t){return n!==e[t]})}function mr(e,r){return typeof r=="function"?r(e):r}function Ir(e){if(e.sheet)return e.sheet;for(var r=0;r<document.styleSheets.length;r++)if(document.styleSheets[r].ownerNode===e)return document.styleSheets[r]}function qr(e){var r=document.createElement("style");return r.setAttribute("data-emotion",e.key),e.nonce!==void 0&&r.setAttribute("nonce",e.nonce),r.appendChild(document.createTextNode("")),r.setAttribute("data-s",""),r}var Dr=function(){function e(n){var t=this;this._insertTag=function(o){var a;t.tags.length===0?t.insertionPoint?a=t.insertionPoint.nextSibling:t.prepend?a=t.container.firstChild:a=t.before:a=t.tags[t.tags.length-1].nextSibling,t.container.insertBefore(o,a),t.tags.push(o)},this.isSpeedy=n.speedy===void 0?!0:n.speedy,this.tags=[],this.ctr=0,this.nonce=n.nonce,this.key=n.key,this.container=n.container,this.prepend=n.prepend,this.insertionPoint=n.insertionPoint,this.before=null}var r=e.prototype;return r.hydrate=function(t){t.forEach(this._insertTag)},r.insert=function(t){this.ctr%(this.isSpeedy?65e3:1)===0&&this._insertTag(qr(this));var o=this.tags[this.tags.length-1];if(this.isSpeedy){var a=Ir(o);try{a.insertRule(t,a.cssRules.length)}catch{}}else o.appendChild(document.createTextNode(t));this.ctr++},r.flush=function(){this.tags.forEach(function(t){return t.parentNode&&t.parentNode.removeChild(t)}),this.tags=[],this.ctr=0},e}(),P="-ms-",de="-moz-",y="-webkit-",hr="comm",Te="rule",Oe="decl",Wr="@import",gr="@keyframes",Fr="@layer",Ur=Math.abs,ge=String.fromCharCode,Gr=Object.assign;function Vr(e,r){return E(e,0)^45?(((r<<2^E(e,0))<<2^E(e,1))<<2^E(e,2))<<2^E(e,3):0}function br(e){return e.trim()}function Br(e,r){return(e=r.exec(e))?e[0]:e}function w(e,r,n){return e.replace(r,n)}function je(e,r){return e.indexOf(r)}function E(e,r){return e.charCodeAt(r)|0}function Y(e,r,n){return e.slice(r,n)}function H(e){return e.length}function He(e){return e.length}function re(e,r){return r.push(e),e}function Kr(e,r){return e.map(r).join("")}var be=1,U=1,vr=0,N=0,C=0,G="";function ve(e,r,n,t,o,a,s){return{value:e,root:r,parent:n,type:t,props:o,children:a,line:be,column:U,length:s,return:""}}function V(e,r){return Gr(ve("",null,null,"",null,null,0),e,{length:-e.length},r)}function Yr(){return C}function Zr(){return C=N>0?E(G,--N):0,U--,C===10&&(U=1,be--),C}function T(){return C=N<vr?E(G,N++):0,U++,C===10&&(U=1,be++),C}function M(){return E(G,N)}function se(){return N}function Q(e,r){return Y(G,e,r)}function Z(e){switch(e){case 0:case 9:case 10:case 13:case 32:return 5;case 33:case 43:case 44:case 47:case 62:case 64:case 126:case 59:case 123:case 125:return 4;case 58:return 3;case 34:case 39:case 40:case 91:return 2;case 41:case 93:return 1}return 0}function yr(e){return be=U=1,vr=H(G=e),N=0,[]}function wr(e){return G="",e}function ce(e){return br(Q(N-1,ze(e===91?e+2:e===40?e+1:e)))}function Jr(e){for(;(C=M())&&C<33;)T();return Z(e)>2||Z(C)>3?"":" "}function Qr(e,r){for(;--r&&T()&&!(C<48||C>102||C>57&&C<65||C>70&&C<97););return Q(e,se()+(r<6&&M()==32&&T()==32))}function ze(e){for(;T();)switch(C){case e:return N;case 34:case 39:e!==34&&e!==39&&ze(C);break;case 40:e===41&&ze(e);break;case 92:T();break}return N}function Xr(e,r){for(;T()&&e+C!==47+10;)if(e+C===42+42&&M()===47)break;return"/*"+Q(r,N-1)+"*"+ge(e===47?e:T())}function en(e){for(;!Z(M());)T();return Q(e,N)}function rn(e){return wr(le("",null,null,null,[""],e=yr(e),0,[0],e))}function le(e,r,n,t,o,a,s,c,l){for(var d=0,u=0,i=s,g=0,_=0,f=0,p=1,S=1,m=1,b=0,x="",j=o,k=a,A=t,v=x;S;)switch(f=b,b=T()){case 40:if(f!=108&&E(v,i-1)==58){je(v+=w(ce(b),"&","&\f"),"&\f")!=-1&&(m=-1);break}case 34:case 39:case 91:v+=ce(b);break;case 9:case 10:case 13:case 32:v+=Jr(f);break;case 92:v+=Qr(se()-1,7);continue;case 47:switch(M()){case 42:case 47:re(nn(Xr(T(),se()),r,n),l);break;default:v+="/"}break;case 123*p:c[d++]=H(v)*m;case 125*p:case 59:case 0:switch(b){case 0:case 125:S=0;case 59+u:m==-1&&(v=w(v,/\f/g,"")),_>0&&H(v)-i&&re(_>32?Ye(v+";",t,n,i-1):Ye(w(v," ","")+";",t,n,i-2),l);break;case 59:v+=";";default:if(re(A=Ke(v,r,n,d,u,o,c,x,j=[],k=[],i),a),b===123)if(u===0)le(v,r,A,A,j,a,i,c,k);else switch(g===99&&E(v,3)===110?100:g){case 100:case 108:case 109:case 115:le(e,A,A,t&&re(Ke(e,A,A,0,0,o,c,x,o,j=[],i),k),o,k,i,c,t?j:k);break;default:le(v,A,A,A,[""],k,0,c,k)}}d=u=_=0,p=m=1,x=v="",i=s;break;case 58:i=1+H(v),_=f;default:if(p<1){if(b==123)--p;else if(b==125&&p++==0&&Zr()==125)continue}switch(v+=ge(b),b*p){case 38:m=u>0?1:(v+="\f",-1);break;case 44:c[d++]=(H(v)-1)*m,m=1;break;case 64:M()===45&&(v+=ce(T())),g=M(),u=i=H(x=v+=en(se())),b++;break;case 45:f===45&&H(v)==2&&(p=0)}}return a}function Ke(e,r,n,t,o,a,s,c,l,d,u){for(var i=o-1,g=o===0?a:[""],_=He(g),f=0,p=0,S=0;f<t;++f)for(var m=0,b=Y(e,i+1,i=Ur(p=s[f])),x=e;m<_;++m)(x=br(p>0?g[m]+" "+b:w(b,/&\f/g,g[m])))&&(l[S++]=x);return ve(e,r,n,o===0?Te:c,l,d,u)}function nn(e,r,n){return ve(e,r,n,hr,ge(Yr()),Y(e,2,-2),0)}function Ye(e,r,n,t){return ve(e,r,n,Oe,Y(e,0,t),Y(e,t+1,-1),t)}function F(e,r){for(var n="",t=He(e),o=0;o<t;o++)n+=r(e[o],o,e,r)||"";return n}function tn(e,r,n,t){switch(e.type){case Fr:if(e.children.length)break;case Wr:case Oe:return e.return=e.return||e.value;case hr:return"";case gr:return e.return=e.value+"{"+F(e.children,t)+"}";case Te:e.value=e.props.join(",")}return H(n=F(e.children,t))?e.return=e.value+"{"+n+"}":""}function an(e){var r=He(e);return function(n,t,o,a){for(var s="",c=0;c<r;c++)s+=e[c](n,t,o,a)||"";return s}}function on(e){return function(r){r.root||(r=r.return)&&e(r)}}function sn(e){var r=Object.create(null);return function(n){return r[n]===void 0&&(r[n]=e(n)),r[n]}}var cn=function(r,n,t){for(var o=0,a=0;o=a,a=M(),o===38&&a===12&&(n[t]=1),!Z(a);)T();return Q(r,N)},ln=function(r,n){var t=-1,o=44;do switch(Z(o)){case 0:o===38&&M()===12&&(n[t]=1),r[t]+=cn(N-1,n,t);break;case 2:r[t]+=ce(o);break;case 4:if(o===44){r[++t]=M()===58?"&\f":"",n[t]=r[t].length;break}default:r[t]+=ge(o)}while(o=T());return r},un=function(r,n){return wr(ln(yr(r),n))},Ze=new WeakMap,fn=function(r){if(!(r.type!=="rule"||!r.parent||r.length<1)){for(var n=r.value,t=r.parent,o=r.column===t.column&&r.line===t.line;t.type!=="rule";)if(t=t.parent,!t)return;if(!(r.props.length===1&&n.charCodeAt(0)!==58&&!Ze.get(t))&&!o){Ze.set(r,!0);for(var a=[],s=un(n,a),c=t.props,l=0,d=0;l<s.length;l++)for(var u=0;u<c.length;u++,d++)r.props[d]=a[l]?s[l].replace(/&\f/g,c[u]):c[u]+" "+s[l]}}},dn=function(r){if(r.type==="decl"){var n=r.value;n.charCodeAt(0)===108&&n.charCodeAt(2)===98&&(r.return="",r.value="")}};function kr(e,r){switch(Vr(e,r)){case 5103:return y+"print-"+e+e;case 5737:case 4201:case 3177:case 3433:case 1641:case 4457:case 2921:case 5572:case 6356:case 5844:case 3191:case 6645:case 3005:case 6391:case 5879:case 5623:case 6135:case 4599:case 4855:case 4215:case 6389:case 5109:case 5365:case 5621:case 3829:return y+e+e;case 5349:case 4246:case 4810:case 6968:case 2756:return y+e+de+e+P+e+e;case 6828:case 4268:return y+e+P+e+e;case 6165:return y+e+P+"flex-"+e+e;case 5187:return y+e+w(e,/(\w+).+(:[^]+)/,y+"box-$1$2"+P+"flex-$1$2")+e;case 5443:return y+e+P+"flex-item-"+w(e,/flex-|-self/,"")+e;case 4675:return y+e+P+"flex-line-pack"+w(e,/align-content|flex-|-self/,"")+e;case 5548:return y+e+P+w(e,"shrink","negative")+e;case 5292:return y+e+P+w(e,"basis","preferred-size")+e;case 6060:return y+"box-"+w(e,"-grow","")+y+e+P+w(e,"grow","positive")+e;case 4554:return y+w(e,/([^-])(transform)/g,"$1"+y+"$2")+e;case 6187:return w(w(w(e,/(zoom-|grab)/,y+"$1"),/(image-set)/,y+"$1"),e,"")+e;case 5495:case 3959:return w(e,/(image-set\([^]*)/,y+"$1$`$1");case 4968:return w(w(e,/(.+:)(flex-)?(.*)/,y+"box-pack:$3"+P+"flex-pack:$3"),/s.+-b[^;]+/,"justify")+y+e+e;case 4095:case 3583:case 4068:case 2532:return w(e,/(.+)-inline(.+)/,y+"$1$2")+e;case 8116:case 7059:case 5753:case 5535:case 5445:case 5701:case 4933:case 4677:case 5533:case 5789:case 5021:case 4765:if(H(e)-1-r>6)switch(E(e,r+1)){case 109:if(E(e,r+4)!==45)break;case 102:return w(e,/(.+:)(.+)-([^]+)/,"$1"+y+"$2-$3$1"+de+(E(e,r+3)==108?"$3":"$2-$3"))+e;case 115:return~je(e,"stretch")?kr(w(e,"stretch","fill-available"),r)+e:e}break;case 4949:if(E(e,r+1)!==115)break;case 6444:switch(E(e,H(e)-3-(~je(e,"!important")&&10))){case 107:return w(e,":",":"+y)+e;case 101:return w(e,/(.+:)([^;!]+)(;|!.+)?/,"$1"+y+(E(e,14)===45?"inline-":"")+"box$3$1"+y+"$2$3$1"+P+"$2box$3")+e}break;case 5936:switch(E(e,r+11)){case 114:return y+e+P+w(e,/[svh]\w+-[tblr]{2}/,"tb")+e;case 108:return y+e+P+w(e,/[svh]\w+-[tblr]{2}/,"tb-rl")+e;case 45:return y+e+P+w(e,/[svh]\w+-[tblr]{2}/,"lr")+e}return y+e+P+e+e}return e}var pn=function(r,n,t,o){if(r.length>-1&&!r.return)switch(r.type){case Oe:r.return=kr(r.value,r.length);break;case gr:return F([V(r,{value:w(r.value,"@","@"+y)})],o);case Te:if(r.length)return Kr(r.props,function(a){switch(Br(a,/(::plac\w+|:read-\w+)/)){case":read-only":case":read-write":return F([V(r,{props:[w(a,/:(read-\w+)/,":"+de+"$1")]})],o);case"::placeholder":return F([V(r,{props:[w(a,/:(plac\w+)/,":"+y+"input-$1")]}),V(r,{props:[w(a,/:(plac\w+)/,":"+de+"$1")]}),V(r,{props:[w(a,/:(plac\w+)/,P+"input-$1")]})],o)}return""})}},_n=[pn],mn=function(r){var n=r.key;if(n==="css"){var t=document.querySelectorAll("style[data-emotion]:not([data-s])");Array.prototype.forEach.call(t,function(p){var S=p.getAttribute("data-emotion");S.indexOf(" ")!==-1&&(document.head.appendChild(p),p.setAttribute("data-s",""))})}var o=r.stylisPlugins||_n,a={},s,c=[];s=r.container||document.head,Array.prototype.forEach.call(document.querySelectorAll('style[data-emotion^="'+n+' "]'),function(p){for(var S=p.getAttribute("data-emotion").split(" "),m=1;m<S.length;m++)a[S[m]]=!0;c.push(p)});var l,d=[fn,dn];{var u,i=[tn,on(function(p){u.insert(p)})],g=an(d.concat(o,i)),_=function(S){return F(rn(S),g)};l=function(S,m,b,x){u=b,_(S?S+"{"+m.styles+"}":m.styles),x&&(f.inserted[m.name]=!0)}}var f={key:n,sheet:new Dr({key:n,container:s,nonce:r.nonce,speedy:r.speedy,prepend:r.prepend,insertionPoint:r.insertionPoint}),nonce:r.nonce,inserted:a,registered:{},insert:l};return f.sheet.hydrate(c),f};function hn(e){for(var r=0,n,t=0,o=e.length;o>=4;++t,o-=4)n=e.charCodeAt(t)&255|(e.charCodeAt(++t)&255)<<8|(e.charCodeAt(++t)&255)<<16|(e.charCodeAt(++t)&255)<<24,n=(n&65535)*1540483477+((n>>>16)*59797<<16),n^=n>>>24,r=(n&65535)*1540483477+((n>>>16)*59797<<16)^(r&65535)*1540483477+((r>>>16)*59797<<16);switch(o){case 3:r^=(e.charCodeAt(t+2)&255)<<16;case 2:r^=(e.charCodeAt(t+1)&255)<<8;case 1:r^=e.charCodeAt(t)&255,r=(r&65535)*1540483477+((r>>>16)*59797<<16)}return r^=r>>>13,r=(r&65535)*1540483477+((r>>>16)*59797<<16),((r^r>>>15)>>>0).toString(36)}var gn={animationIterationCount:1,aspectRatio:1,borderImageOutset:1,borderImageSlice:1,borderImageWidth:1,boxFlex:1,boxFlexGroup:1,boxOrdinalGroup:1,columnCount:1,columns:1,flex:1,flexGrow:1,flexPositive:1,flexShrink:1,flexNegative:1,flexOrder:1,gridRow:1,gridRowEnd:1,gridRowSpan:1,gridRowStart:1,gridColumn:1,gridColumnEnd:1,gridColumnSpan:1,gridColumnStart:1,msGridRow:1,msGridRowSpan:1,msGridColumn:1,msGridColumnSpan:1,fontWeight:1,lineHeight:1,opacity:1,order:1,orphans:1,tabSize:1,widows:1,zIndex:1,zoom:1,WebkitLineClamp:1,fillOpacity:1,floodOpacity:1,stopOpacity:1,strokeDasharray:1,strokeDashoffset:1,strokeMiterlimit:1,strokeOpacity:1,strokeWidth:1},bn=/[A-Z]|^ms/g,vn=/_EMO_([^_]+?)_([^]*?)_EMO_/g,xr=function(r){return r.charCodeAt(1)===45},Je=function(r){return r!=null&&typeof r!="boolean"},we=sn(function(e){return xr(e)?e:e.replace(bn,"-$&").toLowerCase()}),Qe=function(r,n){switch(r){case"animation":case"animationName":if(typeof n=="string")return n.replace(vn,function(t,o,a){return R={name:o,styles:a,next:R},o})}return gn[r]!==1&&!xr(r)&&typeof n=="number"&&n!==0?n+"px":n};function J(e,r,n){if(n==null)return"";if(n.__emotion_styles!==void 0)return n;switch(typeof n){case"boolean":return"";case"object":{if(n.anim===1)return R={name:n.name,styles:n.styles,next:R},n.name;if(n.styles!==void 0){var t=n.next;if(t!==void 0)for(;t!==void 0;)R={name:t.name,styles:t.styles,next:R},t=t.next;var o=n.styles+";";return o}return yn(e,r,n)}case"function":{if(e!==void 0){var a=R,s=n(e);return R=a,J(e,r,s)}break}}if(r==null)return n;var c=r[n];return c!==void 0?c:n}function yn(e,r,n){var t="";if(Array.isArray(n))for(var o=0;o<n.length;o++)t+=J(e,r,n[o])+";";else for(var a in n){var s=n[a];if(typeof s!="object")r!=null&&r[s]!==void 0?t+=a+"{"+r[s]+"}":Je(s)&&(t+=we(a)+":"+Qe(a,s)+";");else if(Array.isArray(s)&&typeof s[0]=="string"&&(r==null||r[s[0]]===void 0))for(var c=0;c<s.length;c++)Je(s[c])&&(t+=we(a)+":"+Qe(a,s[c])+";");else{var l=J(e,r,s);switch(a){case"animation":case"animationName":{t+=we(a)+":"+l+";";break}default:t+=a+"{"+l+"}"}}}return t}var Xe=/label:\s*([^\s;\n{]+)\s*(;|$)/g,R,ke=function(r,n,t){if(r.length===1&&typeof r[0]=="object"&&r[0]!==null&&r[0].styles!==void 0)return r[0];var o=!0,a="";R=void 0;var s=r[0];s==null||s.raw===void 0?(o=!1,a+=J(t,n,s)):a+=s[0];for(var c=1;c<r.length;c++)a+=J(t,n,r[c]),o&&(a+=s[c]);Xe.lastIndex=0;for(var l="",d;(d=Xe.exec(a))!==null;)l+="-"+d[1];var u=hn(a)+l;return{name:u,styles:a,next:R}},wn=!0;function Sr(e,r,n){var t="";return n.split(" ").forEach(function(o){e[o]!==void 0?r.push(e[o]+";"):t+=o+" "}),t}var kn=function(r,n,t){var o=r.key+"-"+n.name;(t===!1||wn===!1)&&r.registered[o]===void 0&&(r.registered[o]=n.styles)},xn=function(r,n,t){kn(r,n,t);var o=r.key+"-"+n.name;if(r.inserted[n.name]===void 0){var a=n;do r.insert(n===a?"."+o:"",a,r.sheet,!0),a=a.next;while(a!==void 0)}};function er(e,r){if(e.inserted[r.name]===void 0)return e.insert("",r,e.sheet,!0)}function rr(e,r,n){var t=[],o=Sr(e,t,n);return t.length<2?n:o+r(t)}var Sn=function(r){var n=mn(r);n.sheet.speedy=function(c){this.isSpeedy=c},n.compat=!0;var t=function(){for(var l=arguments.length,d=new Array(l),u=0;u<l;u++)d[u]=arguments[u];var i=ke(d,n.registered,void 0);return xn(n,i,!1),n.key+"-"+i.name},o=function(){for(var l=arguments.length,d=new Array(l),u=0;u<l;u++)d[u]=arguments[u];var i=ke(d,n.registered),g="animation-"+i.name;return er(n,{name:i.name,styles:"@keyframes "+g+"{"+i.styles+"}"}),g},a=function(){for(var l=arguments.length,d=new Array(l),u=0;u<l;u++)d[u]=arguments[u];var i=ke(d,n.registered);er(n,i)},s=function(){for(var l=arguments.length,d=new Array(l),u=0;u<l;u++)d[u]=arguments[u];return rr(n.registered,t,$n(d))};return{css:t,cx:s,injectGlobal:a,keyframes:o,hydrate:function(l){l.forEach(function(d){n.inserted[d]=!0})},flush:function(){n.registered={},n.inserted={},n.sheet.flush()},sheet:n.sheet,cache:n,getRegisteredStyles:Sr.bind(null,n.registered),merge:rr.bind(null,n.registered,t)}},$n=function e(r){for(var n="",t=0;t<r.length;t++){var o=r[t];if(o!=null){var a=void 0;switch(typeof o){case"boolean":break;case"object":{if(Array.isArray(o))a=e(o);else{a="";for(var s in o)o[s]&&s&&(a&&(a+=" "),a+=s)}break}default:a=o}a&&(n&&(n+=" "),n+=a)}}return n},$r=Sn({key:"css"}),ne=$r.cx,D=$r.css,pe={};const Cn=["acrobat","africa","alaska","albert","albino","album","alcohol","alex","alpha","amadeus","amanda","amazon","america","analog","animal","antenna","antonio","apollo","april","aroma","artist","aspirin","athlete","atlas","banana","bandit","banjo","bikini","bingo","bonus","camera","canada","carbon","casino","catalog","cinema","citizen","cobra","comet","compact","complex","context","credit","critic","crystal","culture","david","delta","dialog","diploma","doctor","domino","dragon","drama","extra","fabric","final","focus","forum","galaxy","gallery","global","harmony","hotel","humor","index","japan","kilo","lemon","liter","lotus","mango","melon","menu","meter","metro","mineral","model","music","object","piano","pirate","plastic","radio","report","signal","sport","studio","subject","super","tango","taxi","tempo","tennis","textile","tokyo","total","tourist","video","visa","academy","alfred","atlanta","atomic","barbara","bazaar","brother","budget","cabaret","cadet","candle","capsule","caviar","channel","chapter","circle","cobalt","comrade","condor","crimson","cyclone","darwin","declare","denver","desert","divide","dolby","domain","double","eagle","echo","eclipse","editor","educate","edward","effect","electra","emerald","emotion","empire","eternal","evening","exhibit","expand","explore","extreme","ferrari","forget","freedom","friday","fuji","galileo","genesis","gravity","habitat","hamlet","harlem","helium","holiday","hunter","ibiza","iceberg","imagine","infant","isotope","jackson","jamaica","jasmine","java","jessica","kitchen","lazarus","letter","license","lithium","loyal","lucky","magenta","manual","marble","maxwell","mayor","monarch","monday","money","morning","mother","mystery","native","nectar","nelson","network","nikita","nobel","nobody","nominal","norway","nothing","number","october","office","oliver","opinion","option","order","outside","package","pandora","panther","papa","pattern","pedro","pencil","people","phantom","philips","pioneer","pluto","podium","portal","potato","process","proxy","pupil","python","quality","quarter","quiet","rabbit","radical","radius","rainbow","ramirez","ravioli","raymond","respect","respond","result","resume","richard","river","roger","roman","rondo","sabrina","salary","salsa","sample","samuel","saturn","savage","scarlet","scorpio","sector","serpent","shampoo","sharon","silence","simple","society","sonar","sonata","soprano","sparta","spider","sponsor","abraham","action","active","actor","adam","address","admiral","adrian","agenda","agent","airline","airport","alabama","aladdin","alarm","algebra","alibi","alice","alien","almond","alpine","amber","amigo","ammonia","analyze","anatomy","angel","annual","answer","apple","archive","arctic","arena","arizona","armada","arnold","arsenal","arthur","asia","aspect","athena","audio","august","austria","avenue","average","axiom","aztec","bagel","baker","balance","ballad","ballet","bambino","bamboo","baron","basic","basket","battery","belgium","benefit","berlin","bermuda","bernard","bicycle","binary","biology","bishop","blitz","block","blonde","bonjour","boris","boston","bottle","boxer","brandy","bravo","brazil","bridge","british","bronze","brown","bruce","bruno","brush","burger","burma","cabinet","cactus","cafe","cairo","calypso","camel","campus","canal","cannon","canoe","cantina","canvas","canyon","capital","caramel","caravan","career","cargo","carlo","carol","carpet","cartel","cartoon","castle","castro","cecilia","cement","center","century","ceramic","chamber","chance","change","chaos","charlie","charm","charter","cheese","chef","chemist","cherry","chess","chicago","chicken","chief","china","cigar","circus","city","clara","classic","claudia","clean","client","climax","clinic","clock","club","cockpit","coconut","cola","collect","colombo","colony","color","combat","comedy","command","company","concert","connect","consul","contact","contour","control","convert","copy","corner","corona","correct","cosmos","couple","courage","cowboy","craft","crash","cricket","crown","cuba","dallas","dance","daniel","decade","decimal","degree","delete","deliver","delphi","deluxe","demand","demo","denmark","derby","design","detect","develop","diagram","diamond","diana","diego","diesel","diet","digital","dilemma","direct","disco","disney","distant","dollar","dolphin","donald","drink","driver","dublin","duet","dynamic","earth","east","ecology","economy","edgar","egypt","elastic","elegant","element","elite","elvis","email","empty","energy","engine","english","episode","equator","escape","escort","ethnic","europe","everest","evident","exact","example","exit","exotic","export","express","factor","falcon","family","fantasy","fashion","fiber","fiction","fidel","fiesta","figure","film","filter","finance","finish","finland","first","flag","flash","florida","flower","fluid","flute","folio","ford","forest","formal","formula","fortune","forward","fragile","france","frank","fresh","friend","frozen","future","gabriel","gamma","garage","garcia","garden","garlic","gemini","general","genetic","genius","germany","gloria","gold","golf","gondola","gong","good","gordon","gorilla","grand","granite","graph","green","group","guide","guitar","guru","hand","happy","harbor","harvard","havana","hawaii","helena","hello","henry","hilton","history","horizon","house","human","icon","idea","igloo","igor","image","impact","import","india","indigo","input","insect","instant","iris","italian","jacket","jacob","jaguar","janet","jargon","jazz","jeep","john","joker","jordan","judo","jumbo","june","jungle","junior","jupiter","karate","karma","kayak","kermit","king","koala","korea","labor","lady","lagoon","laptop","laser","latin","lava","lecture","left","legal","level","lexicon","liberal","libra","lily","limbo","limit","linda","linear","lion","liquid","little","llama","lobby","lobster","local","logic","logo","lola","london","lucas","lunar","machine","macro","madam","madonna","madrid","maestro","magic","magnet","magnum","mailbox","major","mama","mambo","manager","manila","marco","marina","market","mars","martin","marvin","mary","master","matrix","maximum","media","medical","mega","melody","memo","mental","mentor","mercury","message","metal","meteor","method","mexico","miami","micro","milk","million","minimum","minus","minute","miracle","mirage","miranda","mister","mixer","mobile","modem","modern","modular","moment","monaco","monica","monitor","mono","monster","montana","morgan","motel","motif","motor","mozart","multi","museum","mustang","natural","neon","nepal","neptune","nerve","neutral","nevada","news","next","ninja","nirvana","normal","nova","novel","nuclear","numeric","nylon","oasis","observe","ocean","octopus","olivia","olympic","omega","opera","optic","optimal","orange","orbit","organic","orient","origin","orlando","oscar","oxford","oxygen","ozone","pablo","pacific","pagoda","palace","pamela","panama","pancake","panda","panel","panic","paradox","pardon","paris","parker","parking","parody","partner","passage","passive","pasta","pastel","patent","patient","patriot","patrol","pegasus","pelican","penguin","pepper","percent","perfect","perfume","period","permit","person","peru","phone","photo","picasso","picnic","picture","pigment","pilgrim","pilot","pixel","pizza","planet","plasma","plaza","pocket","poem","poetic","poker","polaris","police","politic","polo","polygon","pony","popcorn","popular","postage","precise","prefix","premium","present","price","prince","printer","prism","private","prize","product","profile","program","project","protect","proton","public","pulse","puma","pump","pyramid","queen","radar","ralph","random","rapid","rebel","record","recycle","reflex","reform","regard","regular","relax","reptile","reverse","ricardo","right","ringo","risk","ritual","robert","robot","rocket","rodeo","romeo","royal","russian","safari","salad","salami","salmon","salon","salute","samba","sandra","santana","sardine","school","scoop","scratch","screen","script","scroll","second","secret","section","segment","select","seminar","senator","senior","sensor","serial","service","shadow","sharp","sheriff","shock","short","shrink","sierra","silicon","silk","silver","similar","simon","single","siren","slang","slogan","smart","smoke","snake","social","soda","solar","solid","solo","sonic","source","soviet","special","speed","sphere","spiral","spirit","spring","static","status","stereo","stone","stop","street","strong","student","style","sultan","susan","sushi","suzuki","switch","symbol","system","tactic","tahiti","talent","tarzan","telex","texas","theory","thermos","tiger","titanic","tomato","topic","tornado","toronto","torpedo","totem","tractor","traffic","transit","trapeze","travel","tribal","trick","trident","trilogy","tripod","tropic","trumpet","tulip","tuna","turbo","twist","ultra","uniform","union","uranium","vacuum","valid","vampire","vanilla","vatican","velvet","ventura","venus","vertigo","veteran","victor","vienna","viking","village","vincent","violet","violin","virtual","virus","vision","visitor","visual","vitamin","viva","vocal","vodka","volcano","voltage","volume","voyage","water","weekend","welcome","western","window","winter","wizard","wolf","world","xray","yankee","yoga","yogurt","yoyo","zebra","zero","zigzag","zipper","zodiac","zoom","acid","adios","agatha","alamo","alert","almanac","aloha","andrea","anita","arcade","aurora","avalon","baby","baggage","balloon","bank","basil","begin","biscuit","blue","bombay","botanic","brain","brenda","brigade","cable","calibre","carmen","cello","celtic","chariot","chrome","citrus","civil","cloud","combine","common","cool","copper","coral","crater","cubic","cupid","cycle","depend","door","dream","dynasty","edison","edition","enigma","equal","eric","event","evita","exodus","extend","famous","farmer","food","fossil","frog","fruit","geneva","gentle","george","giant","gilbert","gossip","gram","greek","grille","hammer","harvest","hazard","heaven","herbert","heroic","hexagon","husband","immune","inca","inch","initial","isabel","ivory","jason","jerome","joel","joshua","journal","judge","juliet","jump","justice","kimono","kinetic","leonid","leopard","lima","maze","medusa","member","memphis","michael","miguel","milan","mile","miller","mimic","mimosa","mission","monkey","moral","moses","mouse","nancy","natasha","nebula","nickel","nina","noise","orchid","oregano","origami","orinoco","orion","othello","paper","paprika","prelude","prepare","pretend","promise","prosper","provide","puzzle","remote","repair","reply","rival","riviera","robin","rose","rover","rudolf","saga","sahara","scholar","shelter","ship","shoe","sigma","sister","sleep","smile","spain","spark","split","spray","square","stadium","star","storm","story","strange","stretch","stuart","subway","sugar","sulfur","summer","survive","sweet","swim","table","taboo","target","teacher","telecom","temple","tibet","ticket","tina","today","toga","tommy","tower","trivial","tunnel","turtle","twin","uncle","unicorn","unique","update","valery","vega","version","voodoo","warning","william","wonder","year","yellow","young","absent","absorb","absurd","accent","alfonso","alias","ambient","anagram","andy","anvil","appear","apropos","archer","ariel","armor","arrow","austin","avatar","axis","baboon","bahama","bali","balsa","barcode","bazooka","beach","beast","beatles","beauty","before","benny","betty","between","beyond","billy","bison","blast","bless","bogart","bonanza","book","border","brave","bread","break","broken","bucket","buenos","buffalo","bundle","button","buzzer","byte","caesar","camilla","canary","candid","carrot","cave","chant","child","choice","chris","cipher","clarion","clark","clever","cliff","clone","conan","conduct","congo","costume","cotton","cover","crack","current","danube","data","decide","deposit","desire","detail","dexter","dinner","donor","druid","drum","easy","eddie","enjoy","enrico","epoxy","erosion","except","exile","explain","fame","fast","father","felix","field","fiona","fire","fish","flame","flex","flipper","float","flood","floor","forbid","forever","fractal","frame","freddie","front","fuel","gallop","game","garbo","gate","gelatin","gibson","ginger","giraffe","gizmo","glass","goblin","gopher","grace","gray","gregory","grid","griffin","ground","guest","gustav","gyro","hair","halt","harris","heart","heavy","herman","hippie","hobby","honey","hope","horse","hostel","hydro","imitate","info","ingrid","inside","invent","invest","invite","ivan","james","jester","jimmy","join","joseph","juice","julius","july","kansas","karl","kevin","kiwi","ladder","lake","laura","learn","legacy","legend","lesson","life","light","list","locate","lopez","lorenzo","love","lunch","malta","mammal","margin","margo","marion","mask","match","mayday","meaning","mercy","middle","mike","mirror","modest","morph","morris","mystic","nadia","nato","navy","needle","neuron","never","newton","nice","night","nissan","nitro","nixon","north","oberon","octavia","ohio","olga","open","opus","orca","oval","owner","page","paint","palma","parent","parlor","parole","paul","peace","pearl","perform","phoenix","phrase","pierre","pinball","place","plate","plato","plume","pogo","point","polka","poncho","powder","prague","press","presto","pretty","prime","promo","quest","quick","quiz","quota","race","rachel","raja","ranger","region","remark","rent","reward","rhino","ribbon","rider","road","rodent","round","rubber","ruby","rufus","sabine","saddle","sailor","saint","salt","scale","scuba","season","secure","shake","shallow","shannon","shave","shelf","sherman","shine","shirt","side","sinatra","sincere","size","slalom","slow","small","snow","sofia","song","sound","south","speech","spell","spend","spoon","stage","stamp","stand","state","stella","stick","sting","stock","store","sunday","sunset","support","supreme","sweden","swing","tape","tavern","think","thomas","tictac","time","toast","tobacco","tonight","torch","torso","touch","toyota","trade","tribune","trinity","triton","truck","trust","type","under","unit","urban","urgent","user","value","vendor","venice","verona","vibrate","virgo","visible","vista","vital","voice","vortex","waiter","watch","wave","weather","wedding","wheel","whiskey","wisdom","android","annex","armani","cake","confide","deal","define","dispute","genuine","idiom","impress","include","ironic","null","nurse","obscure","prefer","prodigy","ego","fax","jet","job","rio","ski","yes"];var _e=Cn,nr=function(){return _e[Math.floor(Math.random()*_e.length)]},jn=function(e,r,n){return e.indexOf(r)>-1?!1:e+n+r},tr=function(e){for(var r=_e.length;e>1;)r*=_e.length-e+1,e--;return r},Cr=function(e,r){if(e=e||1,r=r||"-",typeof e!="number")throw new Error("Not A Number!");var n=nr();if(e>1)for(var t=1;t<e;t++){for(var o=!1;!o;)o=jn(n,nr(),r);n=o}return n},zn=function(e,r,n){if(e>tr(r))throw new Error("Max list length for [glue] === "+r+" is "+tr(r));e=e||100,r=r||1,n=n||"-";for(var t=[],o=0;o<e;o++){var a=Cr(r,n);t.indexOf(a)===-1?t.push(a):o--}return t};pe.word=Cr;pe.list=zn;var En=0;function z(e,r,n,t,o,a){var s,c,l={};for(c in r)c=="ref"?s=r[c]:l[c]=r[c];var d={type:e,props:l,key:n,ref:s,__k:null,__:null,__b:0,__e:null,__d:void 0,__c:null,__h:null,constructor:void 0,__v:--En,__source:o,__self:a};if(typeof e=="function"&&(s=e.defaultProps))for(c in s)l[c]===void 0&&(l[c]=s[c]);return h.vnode&&h.vnode(d),d}function An(){const[e,r]=ee(""),[n,t]=ee(""),[o,a]=ee(pe.word(4)),[s,c]=ee(!1);Hr(()=>navigator.clipboard.writeText(o),[o]);const l=Rr(()=>()=>{const d=new URLSearchParams;d.append("ssid",e),d.append("password",n),d.append("token",o),fetch("/save?"+d.toString())},[e,n,o]);return z("div",{class:D({name:"1fa9eb5",styles:"width:100%;transform:translateY(-50%);position:absolute;top:50%"}),children:[z("h1",{class:D({name:"1azakc",styles:"text-align:center"}),children:"Config"}),z("div",{class:ne("content",D({name:"1iyc3e7",styles:"width:300px;margin:0 auto"})),children:z("div",{class:ne("pure-u-1",D({name:"x5deby",styles:"padding:2em;box-sizing:border-box"})),children:z("form",{class:"pure-form pure-form-stacked",children:z("fieldset",{children:[z("label",{for:"ssid",children:"SSID"}),z("input",{class:"pure-input-1",id:"ssid",type:"text",value:e,onInput:d=>r(d.target.value)}),z("label",{for:"password",children:"Password"}),z("input",{class:"pure-input-1",id:"password",type:"password",value:n,onInput:d=>t(d.target.value)}),z("label",{for:"token",children:"Token"}),z("input",{class:"pure-input-1",id:"token",type:"text",value:o,onInput:d=>a(d.target.value)}),z("button",{class:ne("pure-button",D({name:"1d3w5wq",styles:"width:100%"})),type:"button",disabled:s,onClick:()=>a(pe.word(4)),children:"Generate"}),z("br",{}),z("br",{}),z("button",{class:ne("pure-button pure-button-primary",D({name:"1d3w5wq",styles:"width:100%"})),type:"button",disabled:!e||!n||!o||s,onClick:l,children:"Apply"})]})})})})]})}Tr(z(An,{}),document.getElementById("app"));

</script><style>
/*!
Pure v3.0.0
Copyright 2013 Yahoo!
Licensed under the BSD License.
https://github.com/pure-css/pure/blob/master/LICENSE
*//*!
normalize.css v | MIT License | https://necolas.github.io/normalize.css/
Copyright (c) Nicolas Gallagher and Jonathan Neal
*//*! normalize.css v8.0.1 | MIT License | github.com/necolas/normalize.css */html{line-height:1.15;-webkit-text-size-adjust:100%}body{margin:0}main{display:block}h1{font-size:2em;margin:.67em 0}hr{box-sizing:content-box;height:0;overflow:visible}pre{font-family:monospace,monospace;font-size:1em}a{background-color:transparent}abbr[title]{border-bottom:none;text-decoration:underline;-webkit-text-decoration:underline dotted;text-decoration:underline dotted}b,strong{font-weight:bolder}code,kbd,samp{font-family:monospace,monospace;font-size:1em}small{font-size:80%}sub,sup{font-size:75%;line-height:0;position:relative;vertical-align:baseline}sub{bottom:-.25em}sup{top:-.5em}img{border-style:none}button,input,optgroup,select,textarea{font-family:inherit;font-size:100%;line-height:1.15;margin:0}button,input{overflow:visible}button,select{text-transform:none}button,[type=button],[type=reset],[type=submit]{-webkit-appearance:button}button::-moz-focus-inner,[type=button]::-moz-focus-inner,[type=reset]::-moz-focus-inner,[type=submit]::-moz-focus-inner{border-style:none;padding:0}button:-moz-focusring,[type=button]:-moz-focusring,[type=reset]:-moz-focusring,[type=submit]:-moz-focusring{outline:1px dotted ButtonText}fieldset{padding:.35em .75em .625em}legend{box-sizing:border-box;color:inherit;display:table;max-width:100%;padding:0;white-space:normal}progress{vertical-align:baseline}textarea{overflow:auto}[type=checkbox],[type=radio]{box-sizing:border-box;padding:0}[type=number]::-webkit-inner-spin-button,[type=number]::-webkit-outer-spin-button{height:auto}[type=search]{-webkit-appearance:textfield;outline-offset:-2px}[type=search]::-webkit-search-decoration{-webkit-appearance:none}::-webkit-file-upload-button{-webkit-appearance:button;font:inherit}details{display:block}summary{display:list-item}template{display:none}[hidden]{display:none}html{font-family:sans-serif}.hidden,[hidden]{display:none!important}.pure-img{max-width:100%;height:auto;display:block}.pure-g{display:flex;flex-flow:row wrap;align-content:flex-start}.pure-u{display:inline-block;vertical-align:top}.pure-u-1,.pure-u-1-1,.pure-u-1-2,.pure-u-1-3,.pure-u-2-3,.pure-u-1-4,.pure-u-3-4,.pure-u-1-5,.pure-u-2-5,.pure-u-3-5,.pure-u-4-5,.pure-u-5-5,.pure-u-1-6,.pure-u-5-6,.pure-u-1-8,.pure-u-3-8,.pure-u-5-8,.pure-u-7-8,.pure-u-1-12,.pure-u-5-12,.pure-u-7-12,.pure-u-11-12,.pure-u-1-24,.pure-u-2-24,.pure-u-3-24,.pure-u-4-24,.pure-u-5-24,.pure-u-6-24,.pure-u-7-24,.pure-u-8-24,.pure-u-9-24,.pure-u-10-24,.pure-u-11-24,.pure-u-12-24,.pure-u-13-24,.pure-u-14-24,.pure-u-15-24,.pure-u-16-24,.pure-u-17-24,.pure-u-18-24,.pure-u-19-24,.pure-u-20-24,.pure-u-21-24,.pure-u-22-24,.pure-u-23-24,.pure-u-24-24{display:inline-block;letter-spacing:normal;word-spacing:normal;vertical-align:top;text-rendering:auto}.pure-u-1-24{width:4.1667%}.pure-u-1-12,.pure-u-2-24{width:8.3333%}.pure-u-1-8,.pure-u-3-24{width:12.5%}.pure-u-1-6,.pure-u-4-24{width:16.6667%}.pure-u-1-5{width:20%}.pure-u-5-24{width:20.8333%}.pure-u-1-4,.pure-u-6-24{width:25%}.pure-u-7-24{width:29.1667%}.pure-u-1-3,.pure-u-8-24{width:33.3333%}.pure-u-3-8,.pure-u-9-24{width:37.5%}.pure-u-2-5{width:40%}.pure-u-5-12,.pure-u-10-24{width:41.6667%}.pure-u-11-24{width:45.8333%}.pure-u-1-2,.pure-u-12-24{width:50%}.pure-u-13-24{width:54.1667%}.pure-u-7-12,.pure-u-14-24{width:58.3333%}.pure-u-3-5{width:60%}.pure-u-5-8,.pure-u-15-24{width:62.5%}.pure-u-2-3,.pure-u-16-24{width:66.6667%}.pure-u-17-24{width:70.8333%}.pure-u-3-4,.pure-u-18-24{width:75%}.pure-u-19-24{width:79.1667%}.pure-u-4-5{width:80%}.pure-u-5-6,.pure-u-20-24{width:83.3333%}.pure-u-7-8,.pure-u-21-24{width:87.5%}.pure-u-11-12,.pure-u-22-24{width:91.6667%}.pure-u-23-24{width:95.8333%}.pure-u-1,.pure-u-1-1,.pure-u-5-5,.pure-u-24-24{width:100%}.pure-button{display:inline-block;line-height:normal;white-space:nowrap;vertical-align:middle;text-align:center;cursor:pointer;-webkit-user-drag:none;-webkit-user-select:none;user-select:none;box-sizing:border-box}.pure-button::-moz-focus-inner{padding:0;border:0}.pure-button-group{letter-spacing:-.31em;text-rendering:optimizespeed}.opera-only :-o-prefocus,.pure-button-group{word-spacing:-.43em}.pure-button-group .pure-button{letter-spacing:normal;word-spacing:normal;vertical-align:top;text-rendering:auto}.pure-button{font-family:inherit;font-size:100%;padding:.5em 1em;color:#000c;border:none rgba(0,0,0,0);background-color:#e6e6e6;text-decoration:none;border-radius:2px}.pure-button-hover,.pure-button:hover,.pure-button:focus{background-image:linear-gradient(transparent,rgba(0,0,0,.05) 40%,rgba(0,0,0,.1))}.pure-button:focus{outline:0}.pure-button-active,.pure-button:active{box-shadow:0 0 0 1px #00000026 inset,0 0 6px #0003 inset;border-color:#000}.pure-button[disabled],.pure-button-disabled,.pure-button-disabled:hover,.pure-button-disabled:focus,.pure-button-disabled:active{border:none;background-image:none;opacity:.4;cursor:not-allowed;box-shadow:none;pointer-events:none}.pure-button-hidden{display:none}.pure-button-primary,.pure-button-selected,a.pure-button-primary,a.pure-button-selected{background-color:#0078e7;color:#fff}.pure-button-group .pure-button{margin:0;border-radius:0;border-right:1px solid rgba(0,0,0,.2)}.pure-button-group .pure-button:first-child{border-top-left-radius:2px;border-bottom-left-radius:2px}.pure-button-group .pure-button:last-child{border-top-right-radius:2px;border-bottom-right-radius:2px;border-right:none}.pure-form input[type=text],.pure-form input[type=password],.pure-form input[type=email],.pure-form input[type=url],.pure-form input[type=date],.pure-form input[type=month],.pure-form input[type=time],.pure-form input[type=datetime],.pure-form input[type=datetime-local],.pure-form input[type=week],.pure-form input[type=number],.pure-form input[type=search],.pure-form input[type=tel],.pure-form input[type=color],.pure-form select,.pure-form textarea{padding:.5em .6em;display:inline-block;border:1px solid #ccc;box-shadow:inset 0 1px 3px #ddd;border-radius:4px;vertical-align:middle;box-sizing:border-box}.pure-form input:not([type]){padding:.5em .6em;display:inline-block;border:1px solid #ccc;box-shadow:inset 0 1px 3px #ddd;border-radius:4px;box-sizing:border-box}.pure-form input[type=color]{padding:.2em .5em}.pure-form input[type=text]:focus,.pure-form input[type=password]:focus,.pure-form input[type=email]:focus,.pure-form input[type=url]:focus,.pure-form input[type=date]:focus,.pure-form input[type=month]:focus,.pure-form input[type=time]:focus,.pure-form input[type=datetime]:focus,.pure-form input[type=datetime-local]:focus,.pure-form input[type=week]:focus,.pure-form input[type=number]:focus,.pure-form input[type=search]:focus,.pure-form input[type=tel]:focus,.pure-form input[type=color]:focus,.pure-form select:focus,.pure-form textarea:focus{outline:0;border-color:#129fea}.pure-form input:not([type]):focus{outline:0;border-color:#129fea}.pure-form input[type=file]:focus,.pure-form input[type=radio]:focus,.pure-form input[type=checkbox]:focus{outline:thin solid #129FEA;outline:1px auto #129FEA}.pure-form .pure-checkbox,.pure-form .pure-radio{margin:.5em 0;display:block}.pure-form input[type=text][disabled],.pure-form input[type=password][disabled],.pure-form input[type=email][disabled],.pure-form input[type=url][disabled],.pure-form input[type=date][disabled],.pure-form input[type=month][disabled],.pure-form input[type=time][disabled],.pure-form input[type=datetime][disabled],.pure-form input[type=datetime-local][disabled],.pure-form input[type=week][disabled],.pure-form input[type=number][disabled],.pure-form input[type=search][disabled],.pure-form input[type=tel][disabled],.pure-form input[type=color][disabled],.pure-form select[disabled],.pure-form textarea[disabled]{cursor:not-allowed;background-color:#eaeded;color:#cad2d3}.pure-form input:not([type])[disabled]{cursor:not-allowed;background-color:#eaeded;color:#cad2d3}.pure-form input[readonly],.pure-form select[readonly],.pure-form textarea[readonly]{background-color:#eee;color:#777;border-color:#ccc}.pure-form input:focus:invalid,.pure-form textarea:focus:invalid,.pure-form select:focus:invalid{color:#b94a48;border-color:#e9322d}.pure-form input[type=file]:focus:invalid:focus,.pure-form input[type=radio]:focus:invalid:focus,.pure-form input[type=checkbox]:focus:invalid:focus{outline-color:#e9322d}.pure-form select{height:2.25em;border:1px solid #ccc;background-color:#fff}.pure-form select[multiple]{height:auto}.pure-form label{margin:.5em 0 .2em}.pure-form fieldset{margin:0;padding:.35em 0 .75em;border:0}.pure-form legend{display:block;width:100%;padding:.3em 0;margin-bottom:.3em;color:#333;border-bottom:1px solid #e5e5e5}.pure-form-stacked input[type=text],.pure-form-stacked input[type=password],.pure-form-stacked input[type=email],.pure-form-stacked input[type=url],.pure-form-stacked input[type=date],.pure-form-stacked input[type=month],.pure-form-stacked input[type=time],.pure-form-stacked input[type=datetime],.pure-form-stacked input[type=datetime-local],.pure-form-stacked input[type=week],.pure-form-stacked input[type=number],.pure-form-stacked input[type=search],.pure-form-stacked input[type=tel],.pure-form-stacked input[type=color],.pure-form-stacked input[type=file],.pure-form-stacked select,.pure-form-stacked label,.pure-form-stacked textarea{display:block;margin:.25em 0}.pure-form-stacked input:not([type]){display:block;margin:.25em 0}.pure-form-aligned input,.pure-form-aligned textarea,.pure-form-aligned select,.pure-form-message-inline{display:inline-block;vertical-align:middle}.pure-form-aligned textarea{vertical-align:top}.pure-form-aligned .pure-control-group{margin-bottom:.5em}.pure-form-aligned .pure-control-group label{text-align:right;display:inline-block;vertical-align:middle;width:10em;margin:0 1em 0 0}.pure-form-aligned .pure-controls{margin:1.5em 0 0 11em}.pure-form input.pure-input-rounded,.pure-form .pure-input-rounded{border-radius:2em;padding:.5em 1em}.pure-form .pure-group fieldset{margin-bottom:10px}.pure-form .pure-group input,.pure-form .pure-group textarea{display:block;padding:10px;margin:0 0 -1px;border-radius:0;position:relative;top:-1px}.pure-form .pure-group input:focus,.pure-form .pure-group textarea:focus{z-index:3}.pure-form .pure-group input:first-child,.pure-form .pure-group textarea:first-child{top:1px;border-radius:4px 4px 0 0;margin:0}.pure-form .pure-group input:first-child:last-child,.pure-form .pure-group textarea:first-child:last-child{top:1px;border-radius:4px;margin:0}.pure-form .pure-group input:last-child,.pure-form .pure-group textarea:last-child{top:-2px;border-radius:0 0 4px 4px;margin:0}.pure-form .pure-group button{margin:.35em 0}.pure-form .pure-input-1{width:100%}.pure-form .pure-input-3-4{width:75%}.pure-form .pure-input-2-3{width:66%}.pure-form .pure-input-1-2{width:50%}.pure-form .pure-input-1-3{width:33%}.pure-form .pure-input-1-4{width:25%}.pure-form-message-inline{display:inline-block;padding-left:.3em;color:#666;vertical-align:middle;font-size:.875em}.pure-form-message{display:block;color:#666;font-size:.875em}@media only screen and (max-width : 480px){.pure-form button[type=submit]{margin:.7em 0 0}.pure-form input:not([type]),.pure-form input[type=text],.pure-form input[type=password],.pure-form input[type=email],.pure-form input[type=url],.pure-form input[type=date],.pure-form input[type=month],.pure-form input[type=time],.pure-form input[type=datetime],.pure-form input[type=datetime-local],.pure-form input[type=week],.pure-form input[type=number],.pure-form input[type=search],.pure-form input[type=tel],.pure-form input[type=color],.pure-form label{margin-bottom:.3em;display:block}.pure-group input:not([type]),.pure-group input[type=text],.pure-group input[type=password],.pure-group input[type=email],.pure-group input[type=url],.pure-group input[type=date],.pure-group input[type=month],.pure-group input[type=time],.pure-group input[type=datetime],.pure-group input[type=datetime-local],.pure-group input[type=week],.pure-group input[type=number],.pure-group input[type=search],.pure-group input[type=tel],.pure-group input[type=color]{margin-bottom:0}.pure-form-aligned .pure-control-group label{margin-bottom:.3em;text-align:left;display:block;width:100%}.pure-form-aligned .pure-controls{margin:1.5em 0 0}.pure-form-message-inline,.pure-form-message{display:block;font-size:.75em;padding:.2em 0 .8em}}.pure-menu{box-sizing:border-box}.pure-menu-fixed{position:fixed;left:0;top:0;z-index:3}.pure-menu-list,.pure-menu-item{position:relative}.pure-menu-list{list-style:none;margin:0;padding:0}.pure-menu-item{padding:0;margin:0;height:100%}.pure-menu-link,.pure-menu-heading{display:block;text-decoration:none;white-space:nowrap}.pure-menu-horizontal{width:100%;white-space:nowrap}.pure-menu-horizontal .pure-menu-list{display:inline-block}.pure-menu-horizontal .pure-menu-item,.pure-menu-horizontal .pure-menu-heading,.pure-menu-horizontal .pure-menu-separator{display:inline-block;vertical-align:middle}.pure-menu-item .pure-menu-item{display:block}.pure-menu-children{display:none;position:absolute;left:100%;top:0;margin:0;padding:0;z-index:3}.pure-menu-horizontal .pure-menu-children{left:0;top:auto;width:inherit}.pure-menu-allow-hover:hover>.pure-menu-children,.pure-menu-active>.pure-menu-children{display:block;position:absolute}.pure-menu-has-children>.pure-menu-link:after{padding-left:.5em;content:"▸";font-size:small}.pure-menu-horizontal .pure-menu-has-children>.pure-menu-link:after{content:"▾"}.pure-menu-scrollable{overflow-y:scroll;overflow-x:hidden}.pure-menu-scrollable .pure-menu-list{display:block}.pure-menu-horizontal.pure-menu-scrollable .pure-menu-list{display:inline-block}.pure-menu-horizontal.pure-menu-scrollable{white-space:nowrap;overflow-y:hidden;overflow-x:auto;padding:.5em 0}.pure-menu-separator,.pure-menu-horizontal .pure-menu-children .pure-menu-separator{background-color:#ccc;height:1px;margin:.3em 0}.pure-menu-horizontal .pure-menu-separator{width:1px;height:1.3em;margin:0 .3em}.pure-menu-horizontal .pure-menu-children .pure-menu-separator{display:block;width:auto}.pure-menu-heading{text-transform:uppercase;color:#565d64}.pure-menu-link{color:#777}.pure-menu-children{background-color:#fff}.pure-menu-link,.pure-menu-heading{padding:.5em 1em}.pure-menu-disabled{opacity:.5}.pure-menu-disabled .pure-menu-link:hover{background-color:transparent;cursor:default}.pure-menu-active>.pure-menu-link,.pure-menu-link:hover,.pure-menu-link:focus{background-color:#eee}.pure-menu-selected>.pure-menu-link,.pure-menu-selected>.pure-menu-link:visited{color:#000}.pure-table{border-collapse:collapse;border-spacing:0;empty-cells:show;border:1px solid #cbcbcb}.pure-table caption{color:#000;font:italic 85%/1 arial,sans-serif;padding:1em 0;text-align:center}.pure-table td,.pure-table th{border-left:1px solid #cbcbcb;border-width:0 0 0 1px;font-size:inherit;margin:0;overflow:visible;padding:.5em 1em}.pure-table thead{background-color:#e0e0e0;color:#000;text-align:left;vertical-align:bottom}.pure-table td{background-color:transparent}.pure-table-odd td{background-color:#f2f2f2}.pure-table-striped tr:nth-child(2n-1) td{background-color:#f2f2f2}.pure-table-bordered td{border-bottom:1px solid #cbcbcb}.pure-table-bordered tbody>tr:last-child>td{border-bottom-width:0}.pure-table-horizontal td,.pure-table-horizontal th{border-width:0 0 1px 0;border-bottom:1px solid #cbcbcb}.pure-table-horizontal tbody>tr:last-child>td{border-bottom-width:0}

</style></head><body><div id="app" class="pure-g"></div></body></html>
)rawliteral";

void configAP() {
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Configuring device");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(gateway, gateway, subnet);
  WiFi.softAP(ESP_SSID);
  Serial.println("Configuring device done");

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", gateway);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[HTTP] GET / ->" + request->client()->remoteIP().toString());
    request->send_P(200, "text/html", setup_html);
  });

  server.on("/save", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[HTTP] GET /save ->" + request->client()->remoteIP().toString());
    String ssid = request->getParam("ssid")->value();
    String password = request->getParam("password")->value();
    String token = request->getParam("token")->value();

    appConfig.setSSID(const_cast<char *>(ssid.c_str()));
    appConfig.setPassword(const_cast<char *>(password.c_str()));
    appConfig.setToken(const_cast<char *>(token.c_str()));
    appConfig.configSave();
    request->send(200, "text/plain", "OK");

    // Wait for response to be sent
    delay(1000);
    server.reset();
    ESP.restart();
  });
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  EEPROM.begin(512);

  analogWriteRange(1023);

  // Set LED pin as output
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);
  pinMode(HW_RST, INPUT);


  digitalWrite(LED_BUILTIN, HIGH);

  // Load configuration from EEPROM
  Serial.println("Config loading...");
  appConfig.begin();
  Serial.print("SSID: ");
  Serial.println(appConfig.getSSID());
  Serial.print("Password: ");
  Serial.println(appConfig.getPassword());
  Serial.print("Brightness: ");
  Serial.println(appConfig.getBrightness());
  Serial.print("Token: ");
  Serial.println(appConfig.getToken());
  Serial.println("Config loaded");

  lastCLKState = digitalRead(CLK);

  if (strlen(appConfig.getSSID()) == 0) {
    configAP();
  } else {
    Serial.println("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(appConfig.getSSID(), appConfig.getPassword());

    while (WiFi.status() != WL_CONNECTED) {
      delay(250);
      Serial.println("Connecting to WiFi..");
      if (digitalRead(HW_RST) == LOW) {
        Serial.println("Resetting WiFi config");
        char ssid[] = "";
        char password[] = "";
        appConfig.setSSID(ssid);
        appConfig.setPassword(password);
        appConfig.configSave();
        server.reset();
        ESP.restart();
      }
      digitalWrite(LED_BUILTIN, LOW);
      delay(250);
      digitalWrite(LED_BUILTIN, HIGH);
    }
    Serial.println("Connected to the WiFi network");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // // Route for root / web page
    // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    //   request->send_P(200, "text/html", setup_html);
    // });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
      String inputMessage;
      if (request->hasParam("value") && request->hasParam("token")) {
        inputMessage = request->getParam("token")->value();
        if (inputMessage != appConfig.getToken()) {
          request->send(403, "text/plain", "Forbidden");
          return;
        }
        inputMessage = request->getParam("value")->value();
        appConfig.setBrightness(inputMessage.toInt());
        analogWrite(LED_PIN, inputMessage.toInt());
        if (request->hasParam("save")) {
          appConfig.configSave();
        }
      } else {
        request->send(403, "text/plain", "Forbidden");
        return;
      }
      Serial.println(inputMessage);
      request->send(200, "text/plain", "OK");
    });

    server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
      String inputMessage;
      if (request->hasParam("token")) {
        inputMessage = request->getParam("token")->value();
        if (inputMessage != appConfig.getToken()) {
          request->send(403, "text/plain", "Forbidden");
          return;
        }
      } else {
        request->send(403, "text/plain", "Forbidden");
        return;
      }
      Serial.println(inputMessage);
      request->send(200, "text/plain", String(appConfig.getBrightness()));
    });
  }

  analogWrite(LED_PIN, appConfig.getBrightness());

  // Start server
  Serial.println("Server listening");
  server.begin();
}

void loop() {
  currentStateCLK = digitalRead(CLK);
  if (currentStateCLK != lastCLKState) {
    int current = appConfig.getBrightness();
    if (digitalRead(DT) != currentStateCLK) {
      if (current < 96) {
        current += 1;
      } else if (current < 512) {
        current += 5;
      } else if (current < 1003) {
        current += 20;
      } else {
        current = 1023;
      }
      appConfig.setBrightness(current);
      analogWrite(LED_PIN, current);
      lastRotaryUpdate = millis();
    } else {
      if (current > 512) {
        current -= 20;
      } else if (current > 96) {
        current -= 5;
      } else if (current > 0) {
        current -= 1;
      } else {
        current = 0;
      }
      appConfig.setBrightness(current);
      analogWrite(LED_PIN, current);
      lastRotaryUpdate = millis();
    }
    Serial.print("Brightness: ");
    Serial.println(current);
  }

  if (millis() - lastRotaryUpdate > 1000 && lastRotaryUpdate != 0) {
    appConfig.configSave();
    lastRotaryUpdate = 0;
  }

  // Map the counter value to LED brightness (0-255)
  // ledBrightness = map(brightness, 0, 100, 0, 255);
  // ledBrightness = constrain(ledBrightness, 0, 255);
  // analogWrite(LED_PIN, ledBrightness);
  lastCLKState = currentStateCLK;
  int rotaryBtnState = digitalRead(SW);

  // If we detect LOW signal, button is pressed
  if (rotaryBtnState == LOW) {
    // if 50ms have passed since last LOW pulse, it means that the
    // button has been pressed, released and pressed again
    if (millis() - lastRotaryPress > 50) {
      if (appConfig.getBrightness() == 0) {
        analogWrite(LED_PIN, 512);
        appConfig.setBrightness(512);
      } else {
        analogWrite(LED_PIN, 0);
        appConfig.setBrightness(0);
      }
      appConfig.configSave();
      Serial.println("Button pressed!");
    }

    // Remember last button press event
    lastRotaryPress = millis();
  }

  int rstButtonState = digitalRead(HW_RST);

  // If we detect LOW signal, button is pressed
  if (rstButtonState == LOW) {
    // if 50ms have passed since last LOW pulse, it means that the
    // button has been pressed, released and pressed again
    if (millis() - lastRSTPress > 50) {
      Serial.print("Reset button pressed");
      rstCounter++;
    }
    lastRSTPress = millis();
  }

  // Reset counter if button was released for more than 2 seconds
  if (rstCounter > 0 && millis() - lastRSTPress > 2000) {
    Serial.print("Pressed ");
    Serial.print(rstCounter);
    Serial.println(" times ...");
    if (rstCounter == 2) {
      Serial.println("Restarting");
      ESP.restart();
    } else if (rstCounter == 3) {
      Serial.print("SSID: ");
      Serial.println(appConfig.getSSID());
      Serial.print("Password: ");
      Serial.println(appConfig.getPassword());
      Serial.print("Brightness: ");
      Serial.println(appConfig.getBrightness());
      Serial.print("Token: ");
      Serial.println(appConfig.getToken());
      Serial.println("Saving config...");
      appConfig.configSave();
      Serial.println("Config saved!");
    }
    Serial.println("Reset counter");
    rstCounter = 0;
  }

  dnsServer.processNextRequest();

  delay(1);
}
