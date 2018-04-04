#!/usr/bin/env node

const fs = require("fs");
const axios = require("axios");

async function main() {
    const res = await axios.get("http://localhost:3745/health-check");
    console.log(res.data);

    fs.readdirSync(".").forEach(file => {
        if (!file.match(/scoop_.*_.*/))
            return;
        console.log(file);
    });
}

const iter = parseInt(process.argv[2]);
if (isNaN(iter)) {
    console.error(`usage: ${process.argv[1]} <iter>`);
    process.exit(1);
}
main();
