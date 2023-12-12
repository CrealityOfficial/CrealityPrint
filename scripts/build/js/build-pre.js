var fs = require('fs')
var arguments = process.argv.splice(2);
var downloadurl = arguments[0];
fs.readFile('package.json', function (error, data1) {
  if (error) {
    console.log('error')
  } 
	var jsonobj = JSON.parse(data1);
	if(jsonobj.version)
	{
		fs.readFile("CHANGELOG.md", function (error, data2) {
			if(error)
			{
				console.log("read error");
				return;
			}
			
			downloadurl = downloadurl.replace('{VERSION}',jsonobj.version);
			//console.log(data2);
			var content = data2.toString('utf-8').replace("http://download",downloadurl)
			//console.log(content);
			fs.writeFileSync("CHANGELOG.md",content);
			
		});
		
	}
})