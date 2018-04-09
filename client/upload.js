#!/usr/bin/env node

const fs = require("fs");
const axios = require("axios");
const exec = require("child_process").exec;
const PromisePool = require("es6-promise-pool");

const plotdir = process.argv[2];
const iter    = parseInt(process.argv[3]);

if (isNaN(iter)) {
    console.error(`usage: ${process.argv[1]} <plotdir> <iter>`);
    process.exit(1);
}

const generator = create_generator();
const pool = new PromisePool(generator, 10);
pool.start().then(() => console.log("complete"));

function* create_generator() {
    for (let scoop = 0; scoop < 4096; scoop++) {
        // calculate filename
        const fname = `${plotdir}/scoop_${scoop}_5801048965275211042`;

        // lock scoop
        // (also return promise)
        if (fs.existsSync(fname)) {
            console.log(fname);

            yield axios.post("http://localhost:3745/lock", {iter, scoop})
              .catch(e => {
                  // already locked, just move on
                  if (e.response.status == 409) {
                      console.log(`scoop #${scoop} already locked...`);
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

                  return axios.post("http://localhost:3745/unlock", {scoop, link, iter})
                    .catch(e => console.log(e.response.data));
              }).then(res => {
                  if (res == "skip")
                      return res;

                  console.log(`removing ${fname}...`);
                  fs.unlinkSync(fname);
              });
        }
    }
}
