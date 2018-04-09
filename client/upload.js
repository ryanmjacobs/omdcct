#!/usr/bin/env node

const fs = require("fs");
const axios = require("axios");
const exec = require("child_process").exec;

async function main(iter) {
    const res = await axios.get("http://localhost:3745/health-check");
    console.log(res.data);

    for (let i = 0; i < 4096; i++) {
        const fname = `scoop_${i}_5801048965275211042`;

        if (!fs.existsSync(`scoop_${i}_5801048965275211042`))
            continue;

        console.log(fname);

        // lock scoop
        const res =
          await axios.post("http://localhost:3745/lock", {iter, scoop: i})
            .catch(e => {
                // already locked, just move on
                if (e.response.status == 409) {
                    console.log(`scoop #${i} already locked...`);
                }
            });

        // locked succesfully! now upload our scoop
        const link = res.data;
        await new Promise((resolve, reject) => {
            exec(`bash ./upload.sh ${fname} ${link}`, function(err, stdout, stderr) {
                console.log("stdout:", stdout);
                if (stderr)
                    console.log("stderr:", stderr);

                if (err)
                    reject();

                resolve();
            });
        });
    }
}

const iter = parseInt(process.argv[2]);
if (isNaN(iter)) {
    console.error(`usage: ${process.argv[1]} <iter>`);
    process.exit(1);
}
main(iter);
