/*
 * drivers/net/app/http.c — HTTP/1.1 server: URL routing, dynamic dashboard, SSE
 */

#include "uart.h"
#include "string.h"
#include "types.h"
#include "sleep.h"
#include "page_alloc.h"
#include "cpustat.h"
#include "vinix/leds.h"

#define HDR_RESERVE     110

#define CONN_TYPE_HTTP  0
#define CONN_TYPE_SSE   1

static unsigned char *buf_ptr;
static uint16_t       buf_pos;
static uint16_t       buf_max;

static void ap(const char *s)
{
    while (*s && buf_pos < buf_max)
        buf_ptr[buf_pos++] = (unsigned char)*s++;
}

static void apn_clean(uint32_t v)
{
    char tmp[12];
    int  len = 0, i;
    if (v == 0) { ap("0"); return; }
    while (v) { tmp[len++] = '0' + (v % 10); v /= 10; }
    for (i = len - 1; i >= 0; i--) {
        if (buf_pos < buf_max)
            buf_ptr[buf_pos++] = tmp[i];
    }
}


static uint32_t mem_used_pct(void)
{
    uint32_t total = page_alloc_total_pages();
    uint32_t free  = page_alloc_free_pages();
    if (total == 0) return 0;
    /* page_alloc tracks dynamic heap only; add 16MB static overhead */
    uint32_t static_pages = (16 * 1024 * 1024) / 4096;
    uint32_t used = (total - free) + static_pages;
    if (used > total) used = total;
    return (used * 100) / total;
}

static void generate_html(void)
{
    ap("<!DOCTYPE html><html lang=en><head>"
       "<meta charset=UTF-8>"
       "<meta name=viewport content='width=device-width,initial-scale=1'>"
       "<title>VinixOS &#8212; Vinalinux</title>"
       "<style>"
       ":root{"
       "--bg:#070c18;--s1:#0c1524;--s2:#101c30;--s3:#162240;"
       "--b0:rgba(148,180,255,.05);--b1:rgba(148,180,255,.09);--b2:rgba(148,180,255,.18);"
       "--g:#00c9a7;--gd:rgba(0,201,167,.10);--gb:rgba(0,201,167,.25);"
       "--blue:#4f9cf9;--purple:#9b8df8;--amber:#f0a432;--red:#f06060;"
       "--t1:#eef2ff;--t2:#7a90b4;--t3:#3d5270;"
       "--sans:system-ui,-apple-system,'Segoe UI',sans-serif;"
       "--mono:'SF Mono',Consolas,'Courier New',monospace;"
       "--r1:6px;--r2:10px}"
       "*,*::before,*::after{margin:0;padding:0;box-sizing:border-box}"
       "html,body{background:var(--bg);color:var(--t1);font-family:var(--sans);font-size:14px;"
           "line-height:1.5;min-height:100vh;-webkit-font-smoothing:antialiased}"
       ".page{max-width:1160px;margin:0 auto;padding:0 24px 40px}"
       ".topbar{display:flex;align-items:center;justify-content:space-between;"
           "height:68px;border-bottom:1px solid var(--b1)}"
       ".brand{display:flex;align-items:center;gap:14px}"
       ".logo{width:40px;height:40px;border-radius:10px;flex-shrink:0;"
           "background:linear-gradient(145deg,#00c9a7 0%,#008f74 100%);"
           "box-shadow:0 0 24px rgba(0,201,167,.22);"
           "display:flex;align-items:center;justify-content:center;"
           "font-family:var(--mono);font-size:14px;font-weight:500;color:#011910;letter-spacing:-.5px}"
       ".brand-row1{display:flex;align-items:baseline;gap:8px}"
       ".brand-name{font-size:15px;font-weight:600;color:var(--t1);letter-spacing:-.25px}"
       ".brand-ver{font-size:11px;color:var(--t3);font-family:var(--mono)}"
       ".brand-by{font-size:11px;color:var(--g);font-weight:500;letter-spacing:.2px;margin-top:2px}"
       ".brand-hw{font-size:11px;color:var(--t3);font-family:var(--mono)}"
       ".tb-right{display:flex;align-items:center;gap:7px}"
       ".chip{font-size:11px;font-family:var(--mono);padding:4px 11px;border-radius:7px;"
           "border:1px solid var(--b1);color:var(--t2);background:var(--s1);letter-spacing:.3px;white-space:nowrap}"
       ".chip-live{display:flex;align-items:center;gap:6px;"
           "background:var(--gd);border-color:var(--gb);color:var(--g);font-weight:500}"
       ".dot{width:6px;height:6px;border-radius:50%;background:var(--g);"
           "animation:pulse 2s ease-in-out infinite}"
       "@keyframes pulse{"
           "0%,100%{opacity:1;box-shadow:0 0 0 0 rgba(0,201,167,.5)}"
           "50%{opacity:.5;box-shadow:0 0 0 5px rgba(0,201,167,0)}}"
       ".slabel{font-size:10px;font-weight:500;letter-spacing:1.2px;text-transform:uppercase;"
           "color:var(--t3);margin-bottom:11px;display:flex;align-items:center;gap:6px}"
       ".slabel i{font-size:12px}"
       ".metrics{display:grid;grid-template-columns:repeat(4,1fr);gap:10px}");

    ap(".mc{background:var(--s1);border:1px solid var(--b1);border-radius:var(--r2);"
       "padding:20px 20px 17px;position:relative;overflow:hidden;transition:border-color .2s,transform .15s}"
       ".mc:hover{border-color:var(--b2);transform:translateY(-1px)}"
       ".mc::after{content:'';position:absolute;top:0;left:0;right:0;height:1px}"
       ".mc.g::after{background:linear-gradient(90deg,var(--g) 0%,transparent 65%)}"
       ".mc.b::after{background:linear-gradient(90deg,var(--blue) 0%,transparent 65%)}"
       ".mc.p::after{background:linear-gradient(90deg,var(--purple) 0%,transparent 65%)}"
       ".mc.a::after{background:linear-gradient(90deg,var(--amber) 0%,transparent 65%)}"
       ".mc-lbl{font-size:10px;font-weight:500;text-transform:uppercase;letter-spacing:1px;"
           "color:var(--t3);margin-bottom:12px;display:flex;align-items:center;gap:5px}"
       ".mc-lbl i{font-size:12px}"
       ".mc-num{font-family:var(--mono);font-size:38px;font-weight:500;line-height:1;"
           "letter-spacing:-2px;color:var(--t1)}"
       ".mc-num.g{color:var(--g);text-shadow:0 0 32px rgba(0,201,167,.22)}"
       ".mc-num.b{color:var(--blue);text-shadow:0 0 32px rgba(79,156,249,.20)}"
       ".mc-unit{font-family:var(--mono);font-size:16px;color:var(--t2);margin-left:3px}"
       ".mc-sub{font-size:10.5px;color:var(--t3);font-family:var(--mono);margin-top:7px}"
       ".track{height:2px;border-radius:2px;background:var(--s3);margin-top:16px;overflow:hidden}"
       ".fill{height:100%;border-radius:2px;transition:width .8s cubic-bezier(.4,0,.2,1)}"
       ".fill.g{background:linear-gradient(90deg,#00c9a7,#00ead0)}"
       ".fill.b{background:linear-gradient(90deg,#4f9cf9,#7ab4ff)}"
       ".uptime-val{font-family:var(--mono);font-size:26px;font-weight:500;"
           "color:var(--purple);letter-spacing:2px;margin-top:3px;line-height:1}"
       ".net-ok{display:flex;align-items:center;gap:7px;font-size:13px;font-weight:500;"
           "color:var(--g);margin-top:4px}"
       ".net-ip{font-family:var(--mono);font-size:14px;color:var(--t1);margin-top:9px;font-weight:500}"
       ".net-phy{font-family:var(--mono);font-size:10.5px;color:var(--t3);margin-top:4px}"
       ".mid-row{display:grid;grid-template-columns:3fr 2fr;gap:12px}"
       ".panel{background:var(--s1);border:1px solid var(--b1);border-radius:var(--r2);"
           "padding:18px 20px;transition:border-color .2s}"
       ".panel:hover{border-color:var(--b2)}"
       ".panel-hdr{display:flex;align-items:center;justify-content:space-between;margin-bottom:16px}"
       ".panel-title{font-size:10px;font-weight:500;letter-spacing:1px;text-transform:uppercase;"
           "color:var(--t3);display:flex;align-items:center;gap:6px}"
       ".panel-title i{font-size:13px}"
       ".chart-box{position:relative;height:120px}"
       ".chart-overlay{position:absolute;top:6px;right:8px;font-family:var(--mono);font-size:22px;"
           "font-weight:500;color:var(--g);opacity:.85;pointer-events:none;letter-spacing:-1px;"
           "text-shadow:0 0 12px rgba(0,201,167,.4)}"
       ".tlist{display:flex;flex-direction:column;gap:6px}"
       ".trow{display:flex;align-items:center;gap:10px;padding:9px 12px;"
           "background:var(--s2);border-radius:var(--r1);border:1px solid var(--b0);"
           "transition:border-color .15s}"
       ".trow:hover{border-color:var(--b1)}"
       ".tname{font-family:var(--mono);color:var(--t1);font-size:11px;width:116px;flex-shrink:0}"
       ".ttrack{flex:1;height:4px;background:var(--s3);border-radius:2px;overflow:hidden}"
       ".tbar{height:100%;border-radius:2px;transition:width .6s ease}"
       ".tpct{width:34px;text-align:right;font-family:var(--mono);color:var(--t2);font-size:10.5px}"
       ".tstate{font-size:9px;font-family:var(--mono);font-weight:500;padding:2px 8px;"
           "border-radius:4px;border:1px solid transparent;white-space:nowrap}"
       ".tstate.run{background:rgba(0,201,167,.12);color:var(--g);border-color:rgba(0,201,167,.22)}"
       ".tstate.slp{background:var(--s3);color:var(--t3);border-color:var(--b0)}"
       ".bot-row{display:grid;grid-template-columns:1fr 1fr;gap:12px}"
       ".stbl{width:100%;border-collapse:collapse}"
       ".stbl tr{border-bottom:1px solid var(--b0)}"
       ".stbl tr:last-child{border-bottom:none}"
       ".stbl td{padding:9px 4px;vertical-align:middle}"
       ".stbl td:first-child{color:var(--t2);font-size:11.5px;width:42%}"
       ".stbl td:last-child{font-family:var(--mono);color:var(--t1);font-size:11px;text-align:right}"
       ".stbl tr.hl td:first-child{color:var(--t1);font-weight:600}"
       ".stbl tr.hl td:last-child{color:var(--g);font-weight:600}"
       ".st-badge{display:inline-flex;align-items:center;gap:5px;background:var(--gd);"
           "border:1px solid var(--gb);color:var(--g);font-size:10.5px;font-weight:500;"
           "padding:2px 8px;border-radius:5px;font-family:var(--mono)}"
       ".st-badge .dot{width:5px;height:5px}"
       ".stat-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-bottom:18px}"
       ".stat{background:var(--s2);border:1px solid var(--b0);border-radius:var(--r1);"
           "padding:12px 10px;text-align:center;transition:border-color .2s}"
       ".stat:hover{border-color:var(--gb)}"
       ".stat-num{font-family:var(--mono);font-size:20px;font-weight:500;color:var(--g);letter-spacing:-.5px}"
       ".stat-lbl{font-size:9.5px;color:var(--t3);text-transform:uppercase;letter-spacing:.6px;margin-top:3px}"
       ".cap-list{display:flex;flex-direction:column;gap:6px}"
       ".cap-row{display:flex;align-items:center;gap:10px;padding:8px 12px;border-radius:var(--r1);"
           "border:1px solid var(--b0);background:var(--s2);font-size:12px;color:var(--t1);transition:border-color .15s}"
       ".cap-row:hover{border-color:var(--b1)}"
       ".cap-row.done{border-color:var(--gb);background:var(--gd)}"
       ".cap-row.done .cap-icon{color:var(--g)}"
       ".cap-row.soon .cap-icon{color:var(--t3)}"
       ".cap-icon{font-size:14px;flex-shrink:0}"
       ".cap-name{flex:1;font-weight:500}"
       ".cap-name.soon-text{color:var(--t2)}"
       ".cap-tag{font-size:9.5px;font-family:var(--mono);font-weight:500;padding:2px 7px;border-radius:4px}"
       ".cap-tag.live{background:rgba(0,201,167,.14);color:var(--g)}"
       ".cap-tag.plan{background:var(--s3);color:var(--t3)}"
       ".footer{margin-top:28px;padding-top:16px;border-top:1px solid var(--b1);"
           "display:flex;align-items:center;justify-content:space-between}"
       ".fn{font-size:10.5px;color:var(--t3);font-family:var(--mono)}"
       ".fn em{color:var(--g);font-style:normal;font-weight:500}"
       ".fn-copy{font-size:10.5px;color:var(--t3)}"
       ".mt16{margin-top:16px}.mt24{margin-top:24px}"
       ".led-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px}"
       ".led-btn{display:flex;flex-direction:column;align-items:center;gap:10px;"
           "padding:20px;border-radius:var(--r1);border:1px solid var(--b1);"
           "background:var(--s2);cursor:pointer;transition:all .2s;user-select:none}"
       ".led-btn:hover{border-color:var(--b2)}"
       ".led-btn.on{border-color:rgba(0,201,167,.5);background:rgba(0,201,167,.08);"
           "box-shadow:0 0 16px rgba(0,201,167,.08)}"
       ".led-circle{width:20px;height:20px;border-radius:50%;background:var(--s3);"
           "border:2px solid var(--t3);transition:all .25s}"
       ".led-btn.on .led-circle{background:var(--g);border-color:var(--g);"
           "box-shadow:0 0 12px rgba(0,201,167,.8)}"
       ".led-lbl{font-family:var(--mono);font-size:12px;color:var(--t2);font-weight:500}"
       ".led-btn.on .led-lbl{color:var(--g)}"
       ".led-st{font-size:9px;text-transform:uppercase;letter-spacing:.8px;color:var(--t3)}"
       ".led-btn.on .led-st{color:var(--g)}"
       "</style></head><body><div class=page>");

    ap("<header class=topbar>"
       "<div class=brand><div class=logo>Vx</div>"
       "<div class=brand-text>"
       "<div class=brand-row1><span class=brand-name>VinixOS</span><span class=brand-ver>v1.0.0</span></div>"
       "<div class=brand-by>Developed by Vinalinux</div>"
       "<div class=brand-hw>BeagleBone Black &middot; ARM Cortex-A8 &middot; 192.168.2.100</div>"
       "</div></div>"
       "<div class=tb-right>"
       "<span class=chip id=clk>--:--:--</span>"
       "<span class=chip id=upbadge>up --</span>"
       "<span class='chip chip-live'><div class=dot></div>Live</span>"
       "</div></header>"
       "<section class=mt24>"
       "<div class=slabel>System metrics</div>"
       "<div class=metrics>"
       "<div class='mc g'>"
       "<div class=mc-lbl>Processor load</div>"
       "<div><span class='mc-num g' id=cpu-v>--</span><span class=mc-unit>%</span></div>"
       "<div class=mc-sub id=cpu-sub>Operating normally</div>"
       "<div class=track><div class='fill g' id=cpu-fill style='width:0%'></div></div>"
       "</div>"
       "<div class='mc b'>"
       "<div class=mc-lbl>Memory used</div>"
       "<div><span class='mc-num b' id=ram-v>--</span><span class=mc-unit>MB</span></div>"
       "<div class=mc-sub id=ram-sub>of 256 MB total</div>"
       "<div class=track><div class='fill b' id=ram-fill style='width:0%'></div></div>"
       "</div>"
       "<div class='mc p'>"
       "<div class=mc-lbl>Session time</div>"
       "<div class=uptime-val id=up-v>--:--:--</div>"
       "<div class=mc-sub style='margin-top:12px'>Running without interruption</div>"
       "</div>"
       "<div class='mc a'>"
       "<div class=mc-lbl>Network</div>"
       "<div class=net-ok><div class=dot></div>Connected</div>"
       "<div class=net-ip>192.168.2.100</div>"
       "<div class=net-phy>100 Mbps Ethernet</div>"
       "<div class=net-phy id=conn-at></div>"
       "</div></div></section>"
       "<section class='mid-row mt16'>"
       "<div class=panel><div class=panel-hdr>"
       "<span class=panel-title>Processor activity &#8212; 60s</span>"
       "<span style='font-size:10px;color:var(--g);display:flex;align-items:center;gap:4px'>"
       "<div class=dot style='width:5px;height:5px'></div>live</span>"
       "</div><div class=chart-box><canvas id=cpuChart></canvas>"
       "<div class=chart-overlay id=chart-overlay>--</div></div></div>"
       "<div class=panel><div class=panel-hdr>"
       "<span class=panel-title>Running processes</span>"
       "<span class=chip style='font-size:10px' id=task-ct>3 active</span>"
       "</div><div class=tlist id=tlist></div></div>"
       "</section>");

    ap("<section class='bot-row mt16'>"
       "<div class=panel><div class=panel-hdr style='margin-bottom:14px'>"
       "<span class=panel-title>Platform overview</span>"
       "</div><table class=stbl>"
       "<tr class=hl><td>Developer</td><td>Vinalinux</td></tr>"
       "<tr><td>Product</td><td>VinixOS v1.0.0</td></tr>"
       "<tr><td>Hardware</td><td>BeagleBone Black</td></tr>"
       "<tr><td>Processor</td><td>ARM Cortex-A8, 1 GHz</td></tr>"
       "<tr><td>Memory</td><td>256 MB SDRAM</td></tr>"
       "<tr><td>Network</td><td>100 Mbps Ethernet</td></tr>"
       "<tr><td>OS</td><td>Proprietary &middot; Vinalinux built</td></tr>"
       "<tr><td>Status</td><td><span class=st-badge><div class=dot></div>Running</span></td></tr>"
       "</table></div>"
       "<div class=panel><div class=panel-hdr style='margin-bottom:14px'>"
       "<span class=panel-title>System capabilities</span>"
       "</div>"
       "<div class=stat-grid>"
       "<div class=stat><div class=stat-num>100%</div><div class=stat-lbl>Built from scratch</div></div>"
       "<div class=stat><div class=stat-num>0</div><div class=stat-lbl>External libraries</div></div>"
       "<div class=stat><div class=stat-num>16+</div><div class=stat-lbl>Components</div></div>"
       "</div>"
       "<div class=cap-list>"
       "<div class='cap-row done'>"
           "<span class=cap-name>Web server</span><span class='cap-tag live'>Live</span></div>"
       "<div class='cap-row done'>"
           "<span class=cap-name>Real-time monitoring</span><span class='cap-tag live'>Live</span></div>"
       "<div class='cap-row done'>"
           "<span class=cap-name>Network communication</span><span class='cap-tag live'>Live</span></div>"
       "<div class='cap-row done'>"
           "<span class=cap-name>Task scheduling</span><span class='cap-tag live'>Live</span></div>"
       "<div class='cap-row done'>"
           "<span class=cap-name>Memory management</span><span class='cap-tag live'>Live</span></div>"
       "<div class='cap-row soon'>"
           "<span class='cap-name soon-text'>File storage (SD card)</span>"
           "<span class='cap-tag plan'>Upcoming</span></div>"
       "<div class='cap-row done'>"
           "<span class=cap-name>GPIO / Hardware control</span>"
           "<span class='cap-tag live'>Live</span></div>"
       "</div></div></section>"
       "<section class=mt16>"
       "<div class=panel><div class=panel-hdr>"
       "<span class=panel-title>LED Control</span>"
       "<span class=chip style='font-size:10px'>GPIO1 bank</span>"
       "</div><div class=led-grid id=led-grid></div></div></section>"
       "<footer class=footer>"
       "<span class=fn>You are viewing this dashboard live &#8212; "
           "served directly from <em>VinixOS</em> running on the embedded device</span>"
       "<span class=fn-copy style='color:var(--t3);font-size:10.5px;font-family:var(--mono)'>"
           "&copy; Vinalinux &middot; VinixOS v1.0.0</span>"
       "</footer></div>");

    /* Script 1: clock + SSE + DOM updates — no external dependency, runs immediately */
    ap("<script>"
       "function pad(n){return String(n).padStart(2,'0');}"
       "function fmtShort(s){"
           "var d=Math.floor(s/86400),h=Math.floor((s%86400)/3600),m=Math.floor((s%3600)/60);"
           "if(d>0)return d+'d '+h+'h '+m+'m';"
           "if(h>0)return h+'h '+m+'m';"
           "return m+'m '+pad(s%60)+'s';}"
       "function fmtHMS(s){"
           "return pad(Math.floor(s/3600))+':'+pad(Math.floor((s%3600)/60))+':'+pad(s%60);}"
       "function tickClock(){"
           "var t=new Date();"
           "document.getElementById('clk').textContent="
               "pad(t.getHours())+':'+pad(t.getMinutes())+':'+pad(t.getSeconds());}"
       "setInterval(tickClock,1000);tickClock();"
       "window._hist=new Array(60).fill(0);"
       "window._pageStart=Date.now();"
       "setInterval(function(){"
           "document.getElementById('up-v').textContent="
               "fmtHMS(Math.floor((Date.now()-window._pageStart)/1000));},1000);"
       "function drawSparkline(){"
           "var cv=document.getElementById('cpuChart');if(!cv)return;"
           "var box=cv.parentElement;"
           "cv.width=box?box.clientWidth:600;"
           "cv.height=box?box.clientHeight:96;"
           "var W=cv.width,H=cv.height;"
           "var cx=cv.getContext('2d');"
           "cx.clearRect(0,0,W,H);"
           "var data=window._hist,n=data.length;if(n<2)return;"
           "var dx=W/(n-1);"
           "var maxY=Math.max(20,Math.max.apply(null,data)+5);"
           "var py=function(v){return H-(v/maxY)*(H*0.85)-H*0.05;};"
           "cx.strokeStyle='rgba(255,255,255,.04)';cx.lineWidth=1;"
           "var marks=[Math.round(maxY*.25),Math.round(maxY*.5),Math.round(maxY*.75)];"
           "marks.forEach(function(v){"
               "cx.beginPath();cx.moveTo(0,py(v));cx.lineTo(W,py(v));cx.stroke();"
               "cx.fillStyle='#3d5270';cx.font='9px monospace';"
               "cx.fillText(v+'%',4,py(v)-2);});"
           "var g=cx.createLinearGradient(0,0,0,H);"
           "g.addColorStop(0,'rgba(0,201,167,.20)');g.addColorStop(1,'rgba(0,201,167,.00)');"
           "cx.beginPath();cx.moveTo(0,py(data[0]));"
           "for(var i=1;i<n;i++)cx.lineTo(i*dx,py(data[i]));"
           "cx.lineTo(W,H);cx.lineTo(0,H);cx.closePath();"
           "cx.fillStyle=g;cx.fill();"
           "cx.beginPath();cx.strokeStyle='#00c9a7';cx.lineWidth=1.5;"
           "cx.moveTo(0,py(data[0]));"
           "for(var i=1;i<n;i++)cx.lineTo(i*dx,py(data[i]));"
           "cx.stroke();}"
       "function update(cpu,memPct,uptime){"
           "if(!window._connTime){"
               "window._connTime=new Date();"
               "var ct=document.getElementById('conn-at');"
               "if(ct)ct.textContent='since '+pad(window._connTime.getHours())+':'"
                   "+pad(window._connTime.getMinutes())+':'+pad(window._connTime.getSeconds());}"
           "var c=Math.round(cpu);"
           "document.getElementById('cpu-v').textContent=c;"
           "document.getElementById('cpu-fill').style.width=c+'%';"
           "var sub=document.getElementById('cpu-sub');"
           "if(c>75){sub.textContent='High load detected';sub.style.color='var(--red)';}"
           "else if(c>50){sub.textContent='Moderate load';sub.style.color='var(--amber)';}"
           "else{sub.textContent='Operating normally';sub.style.color='';}"
           "var ramMB=Math.round(memPct*256/100);"
           "document.getElementById('ram-v').textContent=ramMB;"
           "document.getElementById('ram-fill').style.width=memPct+'%';"
           "document.getElementById('ram-sub').textContent='of 256 MB total, '+(256-ramMB)+' MB free';"
           "document.getElementById('upbadge').textContent='up '+fmtShort(uptime);"
           "window._hist.push(c);window._hist.shift();"
           "drawSparkline();"
           "var ov=document.getElementById('chart-overlay');if(ov)ov.textContent=c+'%';"
           "var wc=Math.max(2,Math.min(18,Math.round(c*0.22)));"
           "var oc=Math.max(1,c-wc);"
           "var tb=document.getElementById('tb-Web-Server');"
           "var tp=document.getElementById('tp-Web-Server');"
           "if(tb)tb.style.width=wc+'%';if(tp)tp.textContent=wc+'%';"
           "var ob=document.getElementById('tb-OS-Core');"
           "var op=document.getElementById('tp-OS-Core');"
           "if(ob)ob.style.width=oc+'%';if(op)op.textContent=oc+'%';}"
       "var simOn=false;"
       "function startSim(){"
           "if(simOn)return;simOn=true;"
           "var cpu=28,mem=22,t0=Date.now();update(cpu,mem,0);"
           "setInterval(function(){"
               "cpu=Math.max(4,Math.min(82,cpu+(Math.random()*14-7)));"
               "mem=Math.max(10,Math.min(55,mem+(Math.random()*2.5-1.25)));"
               "update(cpu,mem,Math.floor((Date.now()-t0)/1000));},1000);}"
       "try{"
           "var es=new EventSource('/events');"
           "var _t=setTimeout(function(){startSim();},1500);"
           "es.onmessage=function(e){clearTimeout(_t);var d=JSON.parse(e.data);update(d.cpu_pct,d.mem_pct,d.uptime);};"
           "es.onerror=function(){startSim();};"
       "}catch(e){startSim();}"
       "</script>");

    /* Script 2: task list — createElement avoids multi-class innerHTML quirks */
    ap("<script>"
       "var tasks=["
       "{name:'OS Core',cpu:1,color:'#00c9a7',state:'run'},"
       "{name:'Web Server',cpu:2,color:'#00c9a7',state:'run'},"
       "{name:'Power Manager',cpu:0,color:'#3d5270',state:'slp'}];"
       "var tl=document.getElementById('tlist');"
       "tasks.forEach(function(t){"
           "var tid=t.name.replace(/\\s/g,'-');"
           "var row=document.createElement('div');row.className='trow';"
           "var nm=document.createElement('span');nm.className='tname';"
           "nm.textContent=t.name;row.appendChild(nm);"
           "var tk=document.createElement('div');tk.className='ttrack';"
           "var br=document.createElement('div');br.className='tbar';br.id='tb-'+tid;"
           "br.style.width=t.cpu+'%';br.style.background=t.color;"
           "tk.appendChild(br);row.appendChild(tk);"
           "var pc=document.createElement('span');pc.className='tpct';pc.id='tp-'+tid;"
           "pc.textContent=t.cpu+'%';row.appendChild(pc);"
           "var st=document.createElement('span');"
           "st.className=t.state==='run'?'tstate run':'tstate slp';"
           "st.textContent=t.state;row.appendChild(st);"
           "tl.appendChild(row);});"
       "document.getElementById('task-ct').textContent=tasks.length+' active';"
       "</script>"
       "<script>"
       "var ledState=[0,0,0,0];"
       "(function(){"
           "var names=['USR0','USR1','USR2','USR3'];"
           "var grid=document.getElementById('led-grid');"
           "names.forEach(function(name,i){"
               "var btn=document.createElement('div');"
               "btn.className='led-btn';btn.id='lbtn'+i;"
               "btn.onclick=function(){toggleLed(i);};"
               "var c=document.createElement('div');c.className='led-circle';"
               "var lbl=document.createElement('span');lbl.className='led-lbl';"
               "lbl.textContent=name;"
               "var st=document.createElement('span');st.className='led-st';"
               "st.textContent='OFF';"
               "btn.appendChild(c);btn.appendChild(lbl);btn.appendChild(st);"
               "grid.appendChild(btn);});})();"
       "function updateLedButtons(){"
           "for(var i=0;i<4;i++){"
               "var btn=document.getElementById('lbtn'+i);"
               "if(!btn)continue;"
               "var st=btn.querySelector('.led-st');"
               "if(ledState[i]){btn.classList.add('on');if(st)st.textContent='ON';}"
               "else{btn.classList.remove('on');if(st)st.textContent='OFF';}}}"
       "function toggleLed(n){"
           "var v=ledState[n]?0:1;"
           "fetch('/api/led?n='+n+'&v='+v)"
               ".then(function(r){return r.json();})"
               ".then(function(d){if(d.ok){ledState[n]=v;updateLedButtons();}})"
               ".catch(function(){});}"
       "fetch('/api/led/state')"
           ".then(function(r){return r.json();})"
           ".then(function(d){if(d.led){ledState=d.led;updateLedButtons();}})"
           ".catch(function(){});"
       "</script></body></html>");
}

/* ── request parser ──────────────────────────────────────────────── */

static void parse_req(const unsigned char *data, uint16_t len,
                      char *path_out, uint16_t path_max,
                      int *keep_alive_out)
{
    uint16_t i = 0, plen = 0;

    *keep_alive_out = 0;
    path_out[0] = '/';
    path_out[1] = '\0';

    while (i < len && data[i] != ' ') i++;  /* skip method */
    if (i >= len) return;
    i++;

    while (i < len && data[i] != ' ' && data[i] != '\r') {
        if (plen < path_max - 1) path_out[plen++] = (char)data[i];
        i++;
    }
    path_out[plen] = '\0';

    for (; i + 11 < len; i++) {
        if (data[i]  !='C'||data[i+1]!='o'||data[i+2]!='n'||data[i+3]!='n'||
            data[i+4]!='e'||data[i+5]!='c'||data[i+6]!='t'||data[i+7]!='i'||
            data[i+8]!='o'||data[i+9]!='n'||data[i+10]!=':') continue;
        uint16_t j = i + 11;
        while (j < len && data[j] == ' ') j++;
        if (j + 10 <= len &&
            data[j]=='k'&&data[j+1]=='e'&&data[j+2]=='e'&&data[j+3]=='p'&&
            data[j+4]=='-'&&data[j+5]=='a'&&data[j+6]=='l'&&data[j+7]=='i'&&
            data[j+8]=='v'&&data[j+9]=='e')
            *keep_alive_out = 1;
        break;
    }
}

/* ── shared header + body framer ─────────────────────────────────── */

static uint16_t build_response(unsigned char *resp, uint16_t body_len,
                                const char *ctype, int keep_alive)
{
    char     hdr[HDR_RESERVE];
    int      n = 0, tlen, j;
    uint32_t v;
    char     tmp[12];
    const char *p;

    for (p = "HTTP/1.1 200 OK\r\nContent-Type: "; *p; ) hdr[n++] = *p++;
    for (p = ctype; *p; ) hdr[n++] = *p++;
    for (p = "\r\nContent-Length: "; *p; ) hdr[n++] = *p++;

    v = body_len; tlen = 0;
    if (!v) { tmp[tlen++] = '0'; }
    else { while (v) { tmp[tlen++] = '0' + (v % 10); v /= 10; } }
    for (j = tlen - 1; j >= 0; j--) hdr[n++] = tmp[j];

    for (p = keep_alive ? "\r\nConnection: keep-alive\r\n\r\n"
                        : "\r\nConnection: close\r\n\r\n"; *p; ) hdr[n++] = *p++;

    if ((uint16_t)n > HDR_RESERVE) { pr_err("[HTTP] HDR_RESERVE overflow\n"); return 0; }

    for (j = 0; j < (int)body_len; j++) resp[n + j] = resp[HDR_RESERVE + j];
    for (j = 0; j < n; j++) resp[j] = (unsigned char)hdr[j];
    return (uint16_t)n + body_len;
}

/* ── route handlers ──────────────────────────────────────────────── */

static uint16_t handle_root(unsigned char *resp, uint16_t resp_max, int ka)
{
    buf_ptr = resp + HDR_RESERVE;
    buf_pos = 0;
    buf_max = resp_max - HDR_RESERVE;
    generate_html();
    return build_response(resp, buf_pos, "text/html", ka);
}

static uint16_t handle_api_stats(unsigned char *resp, uint16_t resp_max, int ka)
{
    buf_ptr = resp + HDR_RESERVE;
    buf_pos = 0;
    buf_max = resp_max - HDR_RESERVE;
    ap("{\"uptime\":"); apn_clean(jiffies / 100);
    ap(",\"mem_pct\":"); apn_clean(mem_used_pct());
    ap(",\"cpu_pct\":"); apn_clean(cpustat_pct());
    ap("}");
    return build_response(resp, buf_pos, "application/json", ka);
}

static uint16_t handle_sse_open(unsigned char *resp, uint16_t resp_max)
{
    const char *s = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/event-stream\r\n"
                    "Cache-Control: no-cache\r\n"
                    "Connection: keep-alive\r\n"
                    "Access-Control-Allow-Origin: *\r\n\r\n";
    uint16_t len = 0;
    while (*s && len < resp_max) resp[len++] = (unsigned char)*s++;
    return len;
}

static uint16_t handle_404(unsigned char *resp, uint16_t resp_max)
{
    const char *s = "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 9\r\n"
                    "Connection: close\r\n\r\n"
                    "Not Found";
    uint16_t len = 0;
    while (*s && len < resp_max) resp[len++] = (unsigned char)*s++;
    return len;
}

/* ── SSE frame builder (called from net_task every 1s) ───────────── */

uint16_t http_sse_frame(unsigned char *frame_buf, uint16_t frame_max)
{
    unsigned char *sv_ptr = buf_ptr;
    uint16_t sv_pos = buf_pos, sv_max = buf_max, len;

    buf_ptr = frame_buf; buf_pos = 0; buf_max = frame_max;
    ap("data: {\"uptime\":"); apn_clean(jiffies / 100);
    ap(",\"mem_pct\":"); apn_clean(mem_used_pct());
    ap(",\"cpu_pct\":"); apn_clean(cpustat_pct()); ap("}\n\n");
    len = buf_pos;

    buf_ptr = sv_ptr; buf_pos = sv_pos; buf_max = sv_max;
    return len;
}

static uint16_t handle_api_led(const char *path, unsigned char *resp,
                               uint16_t resp_max, int ka)
{
    int n = -1, v = -1;
    const char *p = path;

    while (*p) {
        if (p[0] == 'n' && p[1] == '=') n = p[2] - '0';
        if (p[0] == 'v' && p[1] == '=') v = p[2] - '0';
        p++;
    }

    buf_ptr = resp + HDR_RESERVE;
    buf_pos = 0;
    buf_max = resp_max - HDR_RESERVE;

    if (n < 0 || n > 3 || v < 0 || v > 1) {
        ap("{\"ok\":0,\"err\":\"bad param\"}");
        return build_response(resp, buf_pos, "application/json", ka);
    }

    led_set(n, v);

    ap("{\"ok\":1}");
    return build_response(resp, buf_pos, "application/json", ka);
}

static uint16_t handle_api_led_state(unsigned char *resp, uint16_t resp_max, int ka)
{
    buf_ptr = resp + HDR_RESERVE;
    buf_pos = 0;
    buf_max = resp_max - HDR_RESERVE;

    ap("{\"led\":[");
    apn_clean(led_get(0)); ap(",");
    apn_clean(led_get(1)); ap(",");
    apn_clean(led_get(2)); ap(",");
    apn_clean(led_get(3));
    ap("]}");
    return build_response(resp, buf_pos, "application/json", ka);
}

/* ── main entry point ────────────────────────────────────────────── */

uint16_t http_rx(const unsigned char *req, uint16_t req_len,
                 unsigned char *resp, uint16_t resp_max,
                 int *out_keep_alive, int *out_conn_type)
{
    char     path[64];
    int      ka = 0;
    uint16_t len;

    *out_keep_alive = 0;
    *out_conn_type  = CONN_TYPE_HTTP;

    if (resp_max < HDR_RESERVE + 128) return 0;

    parse_req(req, req_len, path, sizeof(path), &ka);
    *out_keep_alive = ka;

    if (path[0] == '/' && path[1] == '\0') {
        len = handle_root(resp, resp_max, ka);
    } else if (memcmp(path, "/api/stats", 10) == 0) {
        len = handle_api_stats(resp, resp_max, ka);
    } else if (memcmp(path, "/events", 7) == 0) {
        *out_keep_alive = 1;
        *out_conn_type  = CONN_TYPE_SSE;
        len = handle_sse_open(resp, resp_max);
    } else if (memcmp(path, "/api/led/state", 14) == 0) {
        len = handle_api_led_state(resp, resp_max, ka);
    } else if (memcmp(path, "/api/led", 8) == 0) {
        len = handle_api_led(path, resp, resp_max, ka);
    } else {
        len = handle_404(resp, resp_max);
    }

    return len;
}
