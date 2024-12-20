const SDK = require('@yuque/sdk');
var moment = require('moment')
var fs = require("fs");
var path = require('path');
const oss = require('ali-oss');
let rawdata = fs.readFileSync(path.join(__dirname,'yuqueconfig.json'));
let yuqueconfig = JSON.parse(rawdata);
const client = new SDK({
  token: process.env.YUQUE
  // other options
});
const store = oss({
    accessKeyId: process.env.KEYID,
    accessKeySecret: process.env.KEYSECRET,
    bucket: 'file-creality',
    region: 'oss-cn-hangzhou'
  });
var arguments = process.argv.splice(2);
var appid = arguments[0].toLowerCase()
var appname = arguments[1]

var config = yuqueconfig[appid.toLowerCase()]
async function prepublish(html,url,version)
{
  var patt1= /[0-9]\.[0-9]+\.[0-9]+.*\([0-9]{4}-.*\)/;
  var res = html.match(patt1);
  var detail = '';
 
  if(res && res.index>=0)
  {
      var next = html.substring(res.index+1);
      var res2 = next.match(patt1);

      if(res2 && res2.index>0)
      {
        detail = html.substring(res.index,res2.index+res.index+1);
      }else{
		detail = html;
		}
  }

  var args = {};
  args.namespace = config.changeLogTemplate.namespace;//"xlvgsc/gkggv8";
  args.slug = config.changeLogTemplate.slug;//"rklawt";
  
  
  
  var doc = await client.docs.get(args);
  
  var body_html = doc.body_html.replace("####VERSION####",detail);
  body_html = body_html.replace("#####DOWNLOADURL########",url);
  body_html = body_html.substring(14,body_html.length);
 
 // var args = {};
args.namespace = config.releaseInnerChangeLog.namespace;//"xlvgsc/cget8s";
args.slug = null;
args.data = {};
args.data.title = appname+" ReleaseNote "+version;
args.data.public = 1
args.data.body = body_html;
//console.log(body_html)
var result = await client.docs.create(args);
console.log(result);
}

async function genLatestJson (argvs) {
    //console.log(argvs)
    var langs = ['en','zh_cn','zh_tw','jp'];
    var args = {};
    args.namespace = config.releaseChangeLog.namespace;//"xlvgsc/lsm275";
    var docs = await client.docs.list(args);
    //console.log(config.releaseChangeLog.namespace)
    var htmls = {};
 for(var i=0;i<docs.length;i++){
     var doc = docs[i];
    //console.log(doc);
    var lang = doc.title.substring(12).toLowerCase();
    var index = langs.indexOf(lang);
    if(index>=0)
    {
        var docArg = {namespace:args.namespace,id:doc.id,slug:doc.slug}; 
        const result = await client.docs.get(docArg);
        htmls[lang] = result.body_html;
        }
    }

        var type = argvs[3].toLowerCase();
	var os = argvs[4].toLowerCase();
	var path  = argvs[5].toLowerCase();
	var md5sum  = argvs[6];
        var versionInfo = {};
        versionInfo.tag_name = argvs[2];
        var dlurl = "https://file-cdn.creality.com/ota-sz/"+appid+"/"+type+"/"+os+"/"+path;
		versionInfo.url = dlurl;
		versionInfo.md5sum = md5sum;
        versionInfo.name = "releaseNote";
        versionInfo.created_at = moment(new Date).format("YYYY-MM-DD HH:mm:ss");
        versionInfo.published_at = moment(new Date).format("YYYY-MM-DD HH:mm:ss");
        versionInfo.release_notes = htmls;
        var key = "latest.json"
        fs.writeFileSync(key,JSON.stringify(versionInfo));
	console.log(htmls);	
        var url = '<a href="'+dlurl+'" target="_blank">'+dlurl+'</a>';
        var body = await prepublish(htmls['zh_cn'],url,versionInfo.tag_name);
		store.put('ota-sz/'+appid+"/"+type+"/"+os+"/latest.json", key).then((result) => {
         console.log(result);
        });

}
if(arguments.length>0)
{
    genLatestJson(arguments).then((ret) => {
        console.log("ret");    // "Saved"
    })
}else{
    console.log("cmd args format: appname version type")
}

