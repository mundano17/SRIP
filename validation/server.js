const express = require('express');
const cors = require('cors');
const path = require('path');

const app = express();
const PORT = 3000;

app.use(cors());
app.use(express.json());

app.use(express.static(__dirname));

let latestColor = null;
let colorHistory = [];
let version = 0;

app.post('/color', (req, res) => {
    const { hex, r, g, b } = req.body;

    if (!hex || r === undefined || g === undefined || b === undefined) {
        return res.status(400).json({
            error: 'missing fields'
        });
    }

    version++;

    const entry = {
        hex,
        r: parseInt(r),
        g: parseInt(g),
        b: parseInt(b),
        timestamp: Date.now(),
        version
    };

    latestColor = entry;

    colorHistory.unshift(entry);

    if (colorHistory.length > 50) {
        colorHistory.pop();
    }

    console.log(
        `[color] ${hex} rgb(${r},${g},${b})`
    );

    res.json({ ok: true });
});

app.get('/color', (req, res) => {
    if (!latestColor) {
        return res.status(204).end();
    }

    res.json(latestColor);
});

app.get('/history', (req, res) => {
    res.json(colorHistory);
});

app.delete('/history', (req, res) => {
    colorHistory = [];
    latestColor = null;

    res.json({ ok: true });
});

app.listen(PORT, () => {
    console.log(`http://localhost:${PORT}`);
});