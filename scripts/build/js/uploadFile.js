var fs = require("fs");
const oss = require('ali-oss');
const store = oss({
    accessKeyId: process.env.KEYID,
    accessKeySecret: process.env.KEYSECRET,
    bucket: 'file-creality',
    region: 'oss-cn-hangzhou'
  });
  var arguments = process.argv.splice(2);
  var appid = arguments[0]
  var appname = arguments[1]
  var type = arguments[2].toLowerCase()
  var ext = arguments[3]
  var path = arguments[4].toLowerCase()
  var filePath = arguments[5]
  var version = arguments[6]
  var os = arguments[7].toLowerCase()
  async function updateFileList()
  {
    const result = await store.list({
        prefix: 'ota-sz/' + appid + '/'+type
      });
      var files = new Array;
      result.objects.forEach(obj => {
          
          var info = {};
          info.name = obj.name.substring(obj.name.lastIndexOf('/')+1);
	  //info.version = version;
          info.created_at = obj.lastModified;
          info.size = obj.size;
          info.url = "https://file-cdn.creality.com/"+obj.name;
          if(info.name.indexOf(ext)>0 && obj.name.lastIndexOf(appid)>0)
          {
            files.push(info);
          }
          
      });
      console.log(files);
      fs.writeFileSync("history.json",JSON.stringify(files));
      store.put('ota-sz/' + appid + "/"+type+"/history.json", "history.json").then((result) => {
         console.log(result);
        });
  }

  
  store.put('ota-sz/' + appid+"/"+type+"/"+os+"/"+path, filePath).then((result) => {
    console.log(result);
    updateFileList().then((ret) => {
        console.log("ret");    // "Saved"
        })
  });
