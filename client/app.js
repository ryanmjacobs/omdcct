#!/usr/bin/env node

const axios = require("axios");

async function main() {
    const res = await axios.get("http://localhost:3745/health-check");
    console.log(res.data);
}

const iter = parseInt(process.argv[2]);

if (isNaN(iter)) {
    console.error(`usage: ${process.argv[1]} <iter>`);
    process.exit(1);
}

main();
