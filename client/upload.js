#!/usr/bin/env node

const fs = require("fs");
const axios = require("axios");
const exec = require("child_process").exec;

async function main(iter) {
    // do a quick service check
    let res = await axios.get("http://localhost:3745/health-check")
               .catch(e => console.log(e.response.data));
    if (!res) return;

    console.log(res.data);

    for (let i = 0; i < 4096; i++) {
        const fname = `scoop_${i}_5801048965275211042`;

        if (!fs.existsSync(`scoop_${i}_5801048965275211042`))
            continue;

        console.log(fname);

        // lock scoop
        axios.post("http://localhost:3745/lock", {iter, scoop: i})
          .catch(e => {
              // already locked, just move on
              if (e.response.status == 409) {
                  console.log(`scoop #${i} already locked...`);
                  return "skip";
              } else {
                  console.log(e.response.status, e.response.data);
                  return "skip"; // uh oh, error not handled right now
              }
              return "good";
          })
          .then(res => {
              if (res == "skip")
                  return res;

              return new Promise((resolve, reject) => {
                  const oldlink = res.data;

                  // locked succesfully! now upload our scoop
                  exec(`bash ./upload.sh ${fname} ${oldlink}`, function(err, stdout, stderr) {
                      if (err) {
                          console.log("stdout:", stdout);
                          console.log("stderr:", stderr);
                          reject();
                      }

                      resolve(stdout.trim());
                  });
              });
          })
          .then(link => {
              if (link == "skip")
                  return link;

              return axios.post("http://localhost:3745/unlock", {scoop:i, link, iter})
                .catch(e => console.log(e.response.data));
          }).then(res => {
              if (res == "skip")
                  return res;

              console.log(`removing ${fname}...`);
              fs.unlinkSync(fname);
          });
    }
}

const iter = parseInt(process.argv[2]);
if (isNaN(iter)) {
    console.error(`usage: ${process.argv[1]} <iter>`);
    process.exit(1);
}

main(iter);
