const SDK = require('@yuque/sdk');
var fs = require("fs");
var path = require('path')
let rawdata = fs.readFileSync(path.join(__dirname,'yuqueconfig.json'));
let yuqueconfig = JSON.parse(rawdata);

const client = new SDK({
  token: process.env.YUQUE
  // other options
});


async function rejectionWithReturnAwait () {
var arguments = process.argv.splice(2);
var appname = arguments[0]
var version = arguments[1]
var config = yuqueconfig[appname.toLowerCase()]

var args = {};
args.namespace = config.prereleaseChangeLog.namespace;//"xlvgsc/pnsl1p";
args.data = {};
args.data.title = appname + " " + version + " 更新日志";
args.data.public = 1
args.data.body = fs.readFileSync('CHANGELOG.md').toString();
console.log(args);
const result = await client.docs.create(args);

}
rejectionWithReturnAwait().then((ret) => {
    console.log("ret");    // "Saved"
})

// apis
//const { users, groups, repos, docs } = client;