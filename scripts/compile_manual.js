// scripts/compile_manual.js
// =====================================================================
// MUSHIN MANUAL COMPILER
// Generates a premium PDF manual from markdown using headless Microsoft Edge.
// Zero dependencies, fully portable.
// =====================================================================

const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const REPO_ROOT = path.resolve(__dirname, '..');
const MARKDOWN_SOURCE = path.join(REPO_ROOT, 'doc', 'features', 'feature_user_manual.md');
const MANUAL_DIR = path.join(REPO_ROOT, 'manual');
const STAGED_PDF = path.join(REPO_ROOT, 'site', 'downloads', 'mushin_manual.pdf');
const TEMP_HTML = path.join(MANUAL_DIR, 'mushin_manual_temp.html');
const OUTPUT_PDF = path.join(MANUAL_DIR, 'mushin_manual.pdf');

console.log('====================================================');
console.log('           MUSHIN PDF MANUAL COMPILER               ');
console.log('====================================================');

// Ensure directories exist
if (!fs.existsSync(MANUAL_DIR)) {
    fs.mkdirSync(MANUAL_DIR, { recursive: true });
}
const siteDownloads = path.join(REPO_ROOT, 'site', 'downloads');
if (!fs.existsSync(siteDownloads)) {
    fs.mkdirSync(siteDownloads, { recursive: true });
}

// 1. Read Markdown
if (!fs.existsSync(MARKDOWN_SOURCE)) {
    console.error(`Error: Source manual markdown not found at ${MARKDOWN_SOURCE}`);
    process.exit(1);
}
const markdown = fs.readFileSync(MARKDOWN_SOURCE, 'utf8');

// 2. Simple regex-based Markdown to HTML parser
function parseMarkdown(md) {
    let html = md;

    // Remove comments
    html = html.replace(/<!--[\s\S]*?-->/g, '');

    // Convert code blocks with syntax styling
    html = html.replace(/```(bash|javascript|css|cpp|html|diff)?\n([\s\S]*?)```/g, (match, lang, code) => {
        return `<pre class="code-block"><code class="language-${lang || 'text'}">${escapeHtml(code.trim())}</code></pre>`;
    });

    // Escape code blocks from other transformations by temporarily stashing them
    const stashedBlocks = [];
    html = html.replace(/<pre[\s\S]*?<\/pre>/g, (match) => {
        stashedBlocks.push(match);
        return `__PRE_STASH_${stashedBlocks.length - 1}__`;
    });

    // Escape inline code blocks `code`
    html = html.replace(/`([^`]+)`/g, '<code class="inline-code">$1</code>');

    // Convert GitHub Alerts (> [!NOTE], > [!TIP], etc.)
    html = html.replace(/>\s*\[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\]\n>\s*([\s\S]*?)(?=\n\n|\n[^\s>])/g, (match, type, content) => {
        const cleanContent = content.replace(/^>\s*/gm, '').trim();
        return `<div class="alert alert-${type.toLowerCase()}">
            <span class="alert-title">${type}</span>
            <p>${cleanContent}</p>
        </div>`;
    });

    // Convert Markdown Tables
    html = html.replace(/(^\|.*\|\n)(^\|[\s:-|]*\|\n)((^\|.*\|\n)+)/gm, (match, header, divider, rows) => {
        const parseRow = (row) => row.split('|').map(cell => cell.trim()).filter((cell, i, arr) => i > 0 && i < arr.length - 1);
        const headers = parseRow(header);
        const tableRows = rows.trim().split('\n').map(row => parseRow(row));

        let tableHtml = '<table><thead><tr>';
        headers.forEach(h => {
            tableHtml += `<th>${h}</th>`;
        });
        tableHtml += '</tr></thead><tbody>';
        tableRows.forEach(row => {
            tableHtml += '<tr>';
            row.forEach(cell => {
                tableHtml += `<td>${cell}</td>`;
            });
            tableHtml += '</tr>';
        });
        tableHtml += '</tbody></table>';
        return tableHtml;
    });

    // Convert headers (Major headers trigger page breaks for premium layout)
    html = html.replace(/^##\s+(.*)$/gm, '<h2 class="section-title">$1</h2>');
    html = html.replace(/^###\s+(.*)$/gm, '<h3 class="subsection-title">$1</h3>');
    html = html.replace(/^####\s+(.*)$/gm, '<h4 class="subsubsection-title">$1</h4>');

    // Convert Bold
    html = html.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');

    // Convert Lists
    // Unordered lists
    html = html.replace(/^\s*-\s+(.*)$/gm, '<li>$1</li>');
    html = html.replace(/(<li>.*<\/li>\n?)+/g, '<ul>$&</ul>');
    // Ordered lists
    html = html.replace(/^\s*\d+\.\s+(.*)$/gm, '<li>$1</li>');
    html = html.replace(/(?<!<\/ul>\n?)(<li>.*<\/li>\n?)+(?!<\/ul>)/g, (match) => {
        if (match.includes('<ul>')) return match; // avoid nesting issues
        return `<ol>${match}</ol>`;
    });

    // Fix lists nested inside lists
    html = html.replace(/<\/ul>\s*<ul>/g, '');
    html = html.replace(/<\/ol>\s*<ol>/g, '');

    // Unstash code blocks
    stashedBlocks.forEach((block, index) => {
        html = html.replace(`__PRE_STASH_${index}__`, block);
    });

    // Paragraph splits (blank lines)
    html = html.split(/\n\n+/).map(p => {
        p = p.trim();
        if (!p) return '';
        if (p.startsWith('<h') || p.startsWith('<pre') || p.startsWith('<div') || p.startsWith('<table>') || p.startsWith('<ul>') || p.startsWith('<ol>')) {
            return p;
        }
        return `<p>${p}</p>`;
    }).join('\n');

    return html;
}

function escapeHtml(str) {
    return str
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
}

// Generate the beautiful HTML manual wrapper
const bodyHtml = parseMarkdown(markdown);

const styledHtmlPage = `<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Mushin MOD-02 User Manual</title>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&family=Outfit:wght@400;600;700;800&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
    <style>
        :root {
            --primary: #00bfff;
            --secondary: #ff9f00;
            --dark-hardware: #111111;
            --text-main: #2d3748;
            --text-light: #718096;
            --panel-border: #e2e8f0;
            --code-bg: #f7fafc;
            --alert-note: #00bfff;
            --alert-warning: #ff3c00;
        }

        @page {
            size: A4;
            margin: 20mm;
            @bottom-center {
                content: counter(page);
            }
        }

        body {
            font-family: 'Inter', sans-serif;
            color: var(--text-main);
            line-height: 1.6;
            font-size: 10.5pt;
            margin: 0;
            padding: 0;
            background: #ffffff;
        }

        /* --- COVER PAGE --- */
        .cover-page {
            page-break-after: always;
            height: 100vh;
            display: flex;
            flex-direction: column;
            justify-content: space-between;
            align-items: center;
            text-align: center;
            box-sizing: border-box;
            padding: 40mm 0 20mm 0;
        }

        .cover-top {
            display: flex;
            flex-direction: column;
            align-items: center;
        }

        .cover-title {
            font-family: 'Outfit', sans-serif;
            font-size: 55pt;
            font-weight: 800;
            letter-spacing: 0.1em;
            color: var(--dark-hardware);
            margin: 0;
            line-height: 1;
        }

        .cover-subtitle {
            font-family: 'Outfit', sans-serif;
            font-size: 16pt;
            font-weight: 600;
            letter-spacing: 0.05em;
            color: var(--text-light);
            margin-top: 15px;
            text-transform: uppercase;
        }

        .cover-badge {
            margin-top: 30px;
            background: var(--dark-hardware);
            color: var(--primary);
            font-family: 'Outfit', sans-serif;
            font-weight: 700;
            font-size: 14pt;
            padding: 10px 30px;
            border-radius: 4px;
            border: 1px solid var(--primary);
            box-shadow: 0 4px 12px rgba(0, 191, 255, 0.15);
        }

        .cover-desc {
            font-family: 'Inter', sans-serif;
            font-weight: 300;
            font-size: 12pt;
            color: var(--text-light);
            max-width: 500px;
            line-height: 1.5;
            margin-top: 50px;
        }

        .cover-footer {
            font-family: 'Outfit', sans-serif;
            font-weight: 600;
            font-size: 12pt;
            letter-spacing: 0.05em;
        }

        .cover-footer span {
            color: var(--primary);
        }

        /* --- CONTENT TYPOGRAPHY --- */
        h1.main-title {
            display: none; /* Handled by cover page */
        }

        .section-title {
            font-family: 'Outfit', sans-serif;
            font-size: 20pt;
            font-weight: 700;
            color: var(--dark-hardware);
            border-bottom: 2px solid var(--panel-border);
            padding-bottom: 8px;
            margin-top: 40px;
            margin-bottom: 20px;
            page-break-before: always;
        }

        /* Prevent page breaks directly after headers */
        h2, h3, h4 {
            page-break-inside: avoid;
            page-break-after: avoid;
        }

        /* Force page break for major modules */
        .section-title:first-of-type {
            page-break-before: avoid; /* Don't page break immediately after cover */
        }

        .subsection-title {
            font-family: 'Outfit', sans-serif;
            font-size: 14pt;
            font-weight: 600;
            color: var(--secondary);
            margin-top: 30px;
            margin-bottom: 12px;
        }

        .subsubsection-title {
            font-family: 'Outfit', sans-serif;
            font-size: 11pt;
            font-weight: 600;
            color: var(--dark-hardware);
            margin-top: 20px;
            margin-bottom: 8px;
        }

        p {
            margin-top: 0;
            margin-bottom: 16px;
            text-align: justify;
        }

        strong {
            font-weight: 600;
            color: var(--dark-hardware);
        }

        ul, ol {
            margin-top: 0;
            margin-bottom: 16px;
            padding-left: 24px;
        }

        li {
            margin-bottom: 6px;
        }

        /* --- CODE & INLINE CODE --- */
        .code-block {
            background: var(--code-bg);
            border: 1px solid var(--panel-border);
            border-left: 4px solid var(--primary);
            border-radius: 4px;
            padding: 15px;
            font-family: 'JetBrains Mono', monospace;
            font-size: 8.5pt;
            line-height: 1.5;
            overflow: hidden;
            margin-bottom: 20px;
            white-space: pre-wrap;
            page-break-inside: avoid;
        }

        .inline-code {
            background: var(--code-bg);
            border: 1px solid var(--panel-border);
            border-radius: 3px;
            padding: 2px 5px;
            font-family: 'JetBrains Mono', monospace;
            font-size: 8.5pt;
            color: #d53f8c;
        }

        /* --- TABLES --- */
        table {
            width: 100%;
            border-collapse: collapse;
            margin-bottom: 25px;
            font-size: 9pt;
            page-break-inside: avoid;
        }

        th, td {
            padding: 8px 12px;
            text-align: left;
            border-bottom: 1px solid var(--panel-border);
        }

        th {
            background-color: var(--dark-hardware);
            color: #ffffff;
            font-family: 'Outfit', sans-serif;
            font-weight: 600;
            font-size: 9pt;
            letter-spacing: 0.05em;
            text-transform: uppercase;
        }

        tr:nth-child(even) td {
            background-color: rgba(0, 0, 0, 0.02);
        }

        /* --- ALERTS --- */
        .alert {
            border: 1px solid var(--panel-border);
            border-left: 4px solid var(--primary);
            border-radius: 4px;
            background: rgba(0, 191, 255, 0.04);
            padding: 15px;
            margin-bottom: 20px;
            page-break-inside: avoid;
        }

        .alert-title {
            font-family: 'Outfit', sans-serif;
            font-weight: 700;
            font-size: 9.5pt;
            letter-spacing: 0.05em;
            color: var(--primary);
            display: block;
            margin-bottom: 5px;
            text-transform: uppercase;
        }

        .alert-warning {
            border-left-color: var(--alert-warning);
            background: rgba(255, 60, 0, 0.04);
        }

        .alert-warning .alert-title {
            color: var(--alert-warning);
        }
        
        .alert-tip {
            border-left-color: var(--secondary);
            background: rgba(255, 159, 0, 0.04);
        }

        .alert-tip .alert-title {
            color: var(--secondary);
        }

        .alert p {
            margin: 0;
            font-size: 9.5pt;
            color: var(--text-main);
        }

        /* --- STYLING SPECIFICS --- */
        .platform-badge {
            display: inline-block;
            background: #edf2f7;
            padding: 2px 8px;
            border-radius: 3px;
            font-size: 8pt;
            font-weight: 600;
        }

    </style>
</head>
<body>

    <!-- COVER PAGE -->
    <div class="cover-page">
        <div class="cover-top">
            <h1 class="cover-title">MUSHIN</h1>
            <div class="cover-subtitle">Dual Filter LFO Matrix Synthesizer</div>
            <div class="cover-badge">MODEL: MOD-02</div>
            <div class="cover-desc">
                An advanced sound processor translating the mathematics of information saturation, momentum kinetics, and trend exhaustion directly into audio processing.
            </div>
        </div>
        <div class="cover-footer">
            <span>MUSHIN AUDIO</span> &copy; 2026 // ALL RIGHTS RESERVED
        </div>
    </div>

    <!-- MAIN BODY -->
    <div class="content-body">
        ${bodyHtml}
    </div>

</body>
</html>`;

fs.writeFileSync(TEMP_HTML, styledHtmlPage, 'utf8');
console.log(`Generated styled HTML template at: ${TEMP_HTML}`);

// 3. Auto-locate Edge executable
function findEdgePath() {
    const paths = [
        'C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe',
        'C:\\Program Files\\Microsoft\\Edge\\Application\\msedge.exe',
        'C:\\Users\\' + process.env.USERNAME + '\\AppData\\Local\\Microsoft\\Edge\\Application\\msedge.exe'
    ];

    for (const p of paths) {
        if (fs.existsSync(p)) {
            return p;
        }
    }

    // Try finding it in path using where.exe
    try {
        const whereResult = execSync('where.exe msedge', { encoding: 'utf8' }).trim();
        if (whereResult) {
            const firstPath = whereResult.split('\n')[0].trim();
            if (fs.existsSync(firstPath)) return firstPath;
        }
    } catch (e) {
        // Ignored
    }

    return null;
}

const edgePath = findEdgePath();

if (!edgePath) {
    console.error('====================================================');
    console.error('Error: Could not locate Microsoft Edge executable on your system.');
    console.error(' Headless print requires Microsoft Edge to render HTML -> PDF.');
    console.error(' Please ensure Microsoft Edge is installed.');
    console.error('====================================================');
    // Cleanup
    if (fs.existsSync(TEMP_HTML)) fs.unlinkSync(TEMP_HTML);
    process.exit(1);
}

console.log(`Located Microsoft Edge at: ${edgePath}`);

// 4. Render HTML to PDF via Headless Edge
console.log('Rendering HTML to PDF using Microsoft Edge...');
const command = `"${edgePath}" --headless --disable-gpu --no-sandbox --print-to-pdf="${OUTPUT_PDF}" "${TEMP_HTML}"`;

try {
    execSync(command);
    console.log(`Successfully rendered PDF manual to: ${OUTPUT_PDF}`);

    // Staging to website downloads
    fs.copyFileSync(OUTPUT_PDF, STAGED_PDF);
    console.log(`Staged PDF manual to website: ${STAGED_PDF}`);

    // Cleanup temp HTML
    if (fs.existsSync(TEMP_HTML)) {
        fs.unlinkSync(TEMP_HTML);
    }

    console.log('====================================================');
    console.log(' Compilation completed successfully!                ');
    console.log('====================================================');
} catch (error) {
    console.error('Failed to execute Edge PDF rendering command.');
    console.error(error.message);
    // Cleanup
    if (fs.existsSync(TEMP_HTML)) fs.unlinkSync(TEMP_HTML);
    process.exit(1);
}
