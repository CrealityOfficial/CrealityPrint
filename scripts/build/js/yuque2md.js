const SDK = require('@yuque/sdk');
var moment = require('moment')
var fs = require("fs");
var path = require('path');
let rawdata = fs.readFileSync(path.join(__dirname,'yuqueconfig.json'));
let yuqueconfig = JSON.parse(rawdata);
const client = new SDK({
  token: process.env.YUQUE
  // other options
});
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
        fs.writeFileSync("doc" + "/" +lang+".md",result.body.replace(/<.*>/,""));
        }
    }
}

var arguments = process.argv.splice(2);
var appname = arguments[0]
var config = yuqueconfig[appname.toLowerCase()]
if(arguments.length>0)
{
    genLatestJson(arguments).then((ret) => {
        console.log("ret");    // "Saved"
    })
}else{
    console.log("cmd args format: appname version type")
}