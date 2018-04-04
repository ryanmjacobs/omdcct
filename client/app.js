#!/usr/bin/env node

const fs = require("fs");
const axios = require("axios");

async function main() {
    const res = await axios.get("http://localhost:3745/health-check");
    console.log(res.data);

    for (let i = 0; i < 4096; i++) {
        const fname = `scoop_${i}_5801048965275211042`;

        if (!fs.existsSync(`scoop_${i}_5801048965275211042`))
            continue;

        console.log(fname);
    }
}

const iter = parseInt(process.argv[2]);
if (isNaN(iter)) {
    console.error(`usage: ${process.argv[1]} <iter>`);
    process.exit(1);
}
main();
