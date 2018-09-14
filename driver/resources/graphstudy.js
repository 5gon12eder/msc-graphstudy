// -*- coding:utf-8; mode:javascript; -*-

// Copyright (C) 2018 Karlsruhe Institute of Technology
// Copyright (C) 2018 Moritz Klammler <moritz.klammler@alumni.kit.edu>
//
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with this program.  If not, see
// <http://www.gnu.org/licenses/>.

function init() {
    initJsOnly();
    initImageErrorHandlers();
    initPrettyNumbers();
    initColumnSorting();
    initColumnHiding();
    initLongConstantNames();
    initPageTitle();
    initAnchors();
    initComparisonRulers();
    initLayoutGliders();
}

function initJsOnly() {
    for (const obj of document.getElementsByClassName('jsonly')) {
        obj.disabled = false;
    }
}

function initImageErrorHandlers() {
    for (const obj of document.getElementsByTagName('IMG')) {
        obj.onerror = function(){
            obj.classList.add('alert');
            obj.setAttribute('alt', "Sorry, cannot display image.");
            obj.classList.remove('pretty');
        };
        obj.setAttribute('src', obj.getAttribute('src'));
    }
}

function initPrettyNumbers() {
    for (const obj of document.getElementsByClassName('number-int')) {
        initSinglePrettyNumber(obj, parseInteger, formatInteger);
    }
    for (const obj of document.getElementsByClassName('number-float')) {
        initSinglePrettyNumber(obj, parseReal, formatFloat);
    }
    for (const obj of document.getElementsByClassName('number-fixed')) {
        initSinglePrettyNumber(obj, parseReal, formatFixed);
    }
    for (const obj of document.getElementsByClassName('number-percent')) {
        initSinglePrettyNumber(obj, parseReal, formatPercent);
    }
    for (const obj of document.getElementsByClassName('number-100')) {
        initSinglePrettyNumber(obj, parseReal, format100);
    }
    for (const obj of document.getElementsByClassName('number-duration')) {
        initSinglePrettyNumber(obj, parseReal, formatDuration);
    }
}

function initSinglePrettyNumber(node, parse, format) {
    const textnode = node.firstChild;
    if (textnode === null) {
        node.setAttribute('data-sort-key', 'NaN');
        return false;
    } else if (textnode.nodeType != Node.TEXT_NODE) {
        console.log("Cannot handle 'number-*' element that has no text node as first (and only) child");
        return false;
    } else {
        const value = parse(textnode.nodeValue);
        if (node.tagName == 'TD') {
            node.setAttribute('data-sort-key', textnode.nodeValue);
        }
        textnode.nodeValue = format(value);
    }
    return true;
}

function initColumnSorting() {
    for (const obj of document.getElementsByTagName('TH')) {
        if (obj.hasAttribute('data-sort-type')) {
            obj.classList.add('clicksort');
            obj.title = "sort by this column";
            obj.onclick = function(){ sortTableRows(obj); };
        }
    }
}

function initColumnHiding() {
    for (const obj of document.getElementsByClassName('hiding')) {
        obj.classList.add('hide');
        obj.title = "hide these columns";
        const what = obj.getAttribute('data-hide')
        obj.onclick = function(){
            if (obj.classList.contains('hide')) {
                obj.classList.remove('hide');
                obj.classList.add('show');
                obj.title = "show these columns";
                for (const item of document.getElementsByClassName(what)) {
                    item.classList.add('hidden');
                }
            } else if (obj.classList.contains('show')) {
                obj.classList.remove('show');
                obj.classList.add('hide');
                obj.title = "hide these columns";
                for (const item of document.getElementsByClassName(what)) {
                    item.classList.remove('hidden');
                }
            } else {
                alert("This is a bug in the web page.");
            }
        };
    }
}

const LONG_CONSTANT_NAMES = {
    'actions' : function(name) {
        switch (name) {
        case 'GRAPHS':     return "Generating and Importing Graphs";
        case 'LAYOUTS':    return "Computing Layouts for Graphs";
        case 'LAY_WORSE':  return "Layout Worsening";
        case 'LAY_INTER':  return "Layout Interpolation";
        case 'PROPERTIES': return "Computing Properties for Layouts";
        case 'METRICS':    return "Computing Metrics for Layouts";
        case 'MODEL':      return "Building, Training and Testing Discriminator Model";
        default: return "Unknown Action";
        };
    },
    'generators' : function(name) {
        switch (name) {
        case 'RANDDAG':     return "Random DAG Collection";
        case 'NORTH':       return "NORTH Collection";
        case 'ROME':        return "ROME Collection";
        case 'IMPORT':      return "Generic Import Source";
        case 'LINDENMAYER': return "Stochastic L-System";
        case 'QUASI':       return "Quasi Crystal";
        case 'GRID':        return "Regular Grid";
        default:            return "Unknown Generator";
        };
    },
    'layouts' : function(name) {
        switch (name) {
        case 'NATIVE':              return "Native Layout";
        case 'FMMM':                return "Fast Multipole Multilevel Layout";
        case 'STRESS':              return "Stress-Minimized Layout";
        case 'DAVIDSON_HAREL':      return "Davidson-Harel Layout";
        case 'SPRING_EMBEDDER_KK':  return "Spring-Embedder Layout (Kamada & Kawai)";
        case 'PIVOT_MDS':           return "Pivit MDS Layout";
        case 'SUGIYAMA':            return "Sugiyama Layout";
        case 'RANDOM_UNIFORM':      return "Random Layout (Uniform Distribution)";
        case 'RANDOM_NORMAL':       return "Random Layout (Normal Distribution)";
        case 'PHANTOM':             return "Phantom Layout";
        default:                    return "Unknown Layout";
        };
    },
    'properties' : function(name) {
        switch (name) {
        case 'ANGULAR':     return "Adjacent Edge Angular Distribution";
        case 'RDF_GLOBAL':  return "Global Radial Distribution Function";
        case 'RDF_LOCAL':   return "Local Radial Distribution Function";
        case 'EDGE_LENGTH': return "Edge Length Distribution";
        case 'PRINCOMP1ST': return "Node Coordinate Distribution / Major Component";
        case 'PRINCOMP2ND': return "Node Coordinate Distribution / Minor Component";
        default:            return "Unknown Property";
        };
    },
    'test-status' : function(name) {
        switch (name) {
        case 'TRUE_POSITIVE':  return "True Positive";
        case 'TRUE_NEGATIVE':  return "True Negative";
        case 'FALSE_POSITIVE': return "False Positive";
        case 'FALSE_NEGATIVE': return "False Negative";
        default:               return "Unknown Test Status";
        };
    },
};

function initLongConstantNames() {
    for (const constant in LONG_CONSTANT_NAMES) {
        const factory = LONG_CONSTANT_NAMES[constant];
        const classname = 'constant-' + constant;
        for (const obj of document.getElementsByClassName(classname)) {
            const textnode = obj.firstChild;
            if (textnode.nodeType !== Node.TEXT_NODE) {
                console.log("Warning: " + obj.tagName + " element of class " + classname
                            + " has no text node as first child as it ought to");
            } else {
                textnode.nodeValue = factory(textnode.nodeValue);
            }
        }
    }
}

function initPageTitle() {
    for (const obj of document.getElementsByTagName('h1')) {
        const textnode = obj.firstChild;
        if (textnode.nodeType === Node.TEXT_NODE) {
            document.title = textnode.nodeValue;
        }
    }
}

function initAnchors() {
    //               1  2  3  4  5  6
    var counters = [ 0, 0, 0, 0, 0, 0 ];
    for (const obj of document.getElementsByTagName('*')) {
        const matches = obj.tagName.match(/H(\d)/);
        if (matches) {
            const level = Number.parseInt(matches[1]);
            if ((level > 0) && (level < counters.length)) {
                counters[level - 1] += 1;
                for (var i = level; i < counters.length; ++i) {
                    counters[i] = 0;
                }
                var label = 'sect-';
                for (var i = 0; i < level; ++i) {
                    label += counters[i] + '.';
                }
                label = label.substring(0, label.length - 1);  // remove dot after last digit
                appendSectionAnchor(obj, obj.hasAttribute('id') ? obj.id : label);
            }
        }
    }
}

function appendSectionAnchor(htag, label) {
    htag.id = label;
    const a = document.createElement('A');
    a.classList.add('anchor');
    a.href = '#' + label;
    a.title = "link to this section";
    a.appendChild(document.createTextNode('\u00A7'));
    htag.appendChild(a);
}

function checkAllCheckBoxes(name, value) {
    for (const obj of document.getElementsByTagName('INPUT')) {
        if (!obj.disabled && (obj.name === name)) {
            obj.checked = value;
        }
    }
}

function updateHistogram(id, url) {
    const obj = document.getElementById(id);
    obj.onerror = function(){
        obj.classList.add('alert');
        obj.setAttribute('alt', "Sorry, cannot display histogram plot.");
    };
    obj.src = url;
    obj.classList.remove('alert');
    obj.setAttribute('alt', "Plot of histogram for property " + obj.getAttribute('data-property-name') + ".");
}

function updateGliding(id, url) {
    const obj = document.getElementById(id);
    obj.onerror = function(){
        obj.classList.add('alert');
        obj.setAttribute('alt', "Sorry, cannot display density plot.");
    };
    obj.src = url;
    obj.classList.remove('alert');
    obj.setAttribute('alt', "Density plot for property " + obj.getAttribute('data-property-name') + ".");
}

function stableSort(array, compare, reversed) {
    const scratch = new Array();
    for (var i = 0; i < array.length; ++i) {
        scratch.push([i, array[i]]);
    }
    const stableCompare = function(lhs, rhs){
        if (compare(lhs[1], rhs[1])) {
            return reversed ? +1 : -1;
        } else if (compare(rhs[1], lhs[1])) {
            return reversed ? -1 : +1;
        } else {
            return lhs[0] - rhs[0];
        }
    };
    scratch.sort(stableCompare);
    for (var i = 0; i < array.length; ++i) {
        array[i] = scratch[i][1];
    }
}

function parseInteger(text) {
    if (text.match(/[+-]?(Infinity|NaN)/)) {
        return Number.parseFloat(text);
    } else {
        return Number.parseInt(text);
    }
}

function parseReal(text) {
    return Number.parseFloat(text);
}

function formatInteger(value) {
    if (!Number.isFinite(value) || !Number.isInteger(value)) {
        console.log("Warning: Non-finite or non-integral integer: " + value);
        return formatReal(value);
    }
    return value.toLocaleString('en-US');
}

function formatReal(value) {
    if (Number.isNaN(value)) {
        return 'NaN';
    } else if (value === Number.POSITIVE_INFINITY) {
        return '+Infinity';
    } else if (value === Number.NEGATIVE_INFINITY) {
        return '-Infinity';
    } else {
        return value.toPrecision(5);
    }
}

function formatFloat(value) {
    if (!Number.isFinite(value)) {
        return formatReal(value);
    }
    const decimals = 3;
    const roundtol = Math.pow(10.0, -2 * decimals - 1);  // prevent displaying 0.9999995 as 1000.000E-03
    var scaled = Math.abs(value);
    var exponent = 0;
    if (value !== 0.0) {
        while (scaled < 1.0 - roundtol) {
            scaled *= 1.0E3;
            exponent -= 3;
        }
        while (scaled > 1000.0) {
            scaled /= 1.0E3;
            exponent += 3;
        }
    }
    const expsig = [ '-', '\u00B1', '+' ][1 + Math.sign(exponent)];
    const expmag = (function(str){ return (str.length < 2) ? '0' + str : str; })(Math.abs(exponent).toFixed());
    return (value < 0 ? '-' : '') + scaled.toFixed(3) + 'E' + expsig + expmag;
}

function formatFixed(value) {
    if (!Number.isFinite(value)) {
        return formatReal(value);
    }
    return value.toFixed(3);
}

function formatPercent(value) {
    if (!Number.isFinite(value) || (value < 0.0) || (value > 1.0)) {
        console.log("Warning: Percentage outside of unit interval: " + value);
        return formatReal(100.0 * value);
    }
    const decimals = 3;
    const margin = Math.pow(10.0, -(2 + decimals));
    var clamped = value;
    if (value !== 0.0) { clamped = Math.max(clamped, 0.0 + margin); }
    if (value !== 1.0) { clamped = Math.min(clamped, 1.0 - margin); }
    return (100.0 * value).toFixed(decimals);
}

function format100(value) {
    return formatFixed(100.0 * value);
}

function formatDuration(value) {
    if (!Number.isFinite(value)) {
        return formatReal(value);
    } else if (value < 0) {
        console.log("Warning: Negative duration: " + value);
        return value.toPrecision(3);
    } else if (value < 1.0E-3) {
        return '< 0.001';
    } else if (value < 60.0) {
        return value.toFixed(3);
    } else {
        const total = Math.round(value)
        const seconds = total % 60;
        const minutes = ((total - seconds) / 60) % 60;
        const hours = (total - 60 * minutes - seconds) / 3600;
        const padded = function(d){
            const temp = '0' + d;
            return temp.substring(temp.length - 2, temp.length);
        };
        return hours + ':' + padded(minutes) + ':' + padded(seconds);
    }
}

function sortTableRows(th) {
    const table = th.parentNode.parentNode;
    const index = th.cellIndex;
    const reversed = th.hasAttribute('data-sort-reversed')
          ? (0 === Number.parseInt(th.getAttribute('data-sort-reversed')))
          : false;
    //console.log("Sorting table data by column " + index + " (reversed = " + reversed + ") ...");
    const datarows = new Array();
    var headercount = 0;
    ROW_LOOP:
    for (const row of table.rows) {
        for (const cell of row.cells) {
            if (cell.tagName == 'TH') {
                headercount += 1;
                continue ROW_LOOP;
            }
        }
        datarows.push(row);
    }
    while (table.rows.length > headercount) { table.deleteRow(-1); }
    const denan = function(x){ return Number.isNaN(x) ? -Infinity : x; };
    const keyconv = function(type){
        if (type === null)     return function(str){ return null; };
        else if (type === 's') return function(str){ return str; };
        else if (type === 'd') return function(str){ return denan(parseInteger(str)); };
        else if (type === 'f') return function(str){ return denan(parseReal(str)); };
        else throw new Exception("Unknown sort key type: " + type);
    }(th.getAttribute('data-sort-type'));
    const accessRaw = function(row) {
        const cell = row.cells.item(index);
        if (cell.hasAttribute('data-sort-key')) {
            return cell.getAttribute('data-sort-key');
        }
        for (var node = row.cells.item(index); node !== null; node = node.firstChild) {
            if (node.nodeType === Node.TEXT_NODE) {
                return node.nodeValue;
            }
        }
    };
    const accessCooked = function(row){ return keyconv(accessRaw(row)); };
    stableSort(datarows, (lhs, rhs) => (accessCooked(lhs) < accessCooked(rhs)), reversed);
    for (const oldrow of datarows) {
        table.appendChild(oldrow);
    }
    th.setAttribute('data-sort-reversed', reversed ? '1' : '0');
}

function initComparisonRulers() {
    const rulers = document.getElementsByClassName('ruler');
    for (var i = 0; i < rulers.length; ++i) {
        initComparisonRuler(rulers[i], i + 1);
    }
}

function initComparisonRuler(ruler, i) {
    const getleft = function(x){ return (5 + 100 * (8 / 9) * (1 + x) / 2).toFixed(10) + "%"; };
    while (ruler.firstChild !== null) {
        ruler.removeChild(ruler.firstChild);
    }
    ruler.style = "";
    for (var pos = -100; pos <= 100; pos += 25) {
        const text = ((pos > 0) ? '+' : '') + pos.toFixed(0);
        const label = document.createElement('DIV');
        const tick = document.createElement('DIV');
        tick.classList.add('tick');
        label.classList.add('label');
        tick.appendChild(document.createTextNode('\uFF5C'));
        label.appendChild(document.createTextNode(text));
        tick.style.left = label.style.left = getleft(pos / 100.0)
        ruler.appendChild(tick);
        ruler.appendChild(label);
    }
    const variations = [
        { 'name' : 'EXPECTED',           'symbol' : '\u25BD', 'delay' : 1.00 },
        { 'name' : 'NN_FORWARD',         'symbol' : '\u25BC', 'delay' : 2.00 },
        { 'name' : 'NN_REVERSE',         'symbol' : '\u25BC', 'delay' : 2.25 },
        { 'name' : 'STRESS_KK',          'symbol' : '\u25C7', 'delay' : 3.00 },
        { 'name' : 'STRESS_FIT_NODESEP', 'symbol' : '\u25C7', 'delay' : 3.25 },
        { 'name' : 'STRESS_FIT_SCALE',   'symbol' : '\u25C7', 'delay' : 3.50 },
        { 'name' : 'HUANG',              'symbol' : '\u25CB', 'delay' : 4.00 },
    ];
    for (var j = 0; j < variations.length; ++j) {
        const attrname = 'data-value-' + variations[j].name;
        if (ruler.hasAttribute(attrname)) {
            const value = parseReal(ruler.getAttribute(attrname));
            if (Number.isFinite(value)) {
                const title = variations[j].name;
                const symbol = variations[j].symbol;
                const delay = variations[j].delay;
                const pin = document.createElement('DIV');
                const kfid = 'pin-slide-' + i.toFixed(0) + '-' + j.toFixed(0);
                pin.classList.add('pin');
                pin.title = title + ': ' + format100(value);
                pin.appendChild(document.createTextNode(symbol));
                pin.style.left = getleft(value);
                pin.style.animation = kfid + " " + (delay).toFixed(3) + "s ease-out 1";
                ruler.appendChild(pin);
                const css = "@keyframes " + kfid + " {\n"
                      + "      0% { left: " + getleft(0.00 * value) + "; }\n"
                      + "    100% { left: " + getleft(1.00 * value) + "; }\n"
                      + "}";
                const stylesheet = document.createElement('STYLE');
                stylesheet.appendChild(document.createTextNode(css));
                document.head.appendChild(stylesheet);
            }
        }
    }
}

function initLayoutGliders() {
    for (const glider of document.getElementsByClassName('layout-glider')) {
        glider.classList.add('glider');
        if (glider.tagName !== 'UL') {
            console.log("WARNING: Ignoring '" + glider.tagName + "' element of class 'layout-glider'");
            continue;
        }
        for (const li of glider.getElementsByTagName('LI')) {
            const rate = parseReal(li.getAttribute('data-rate'));
            li.style = "left:" + (100.0 * rate).toFixed(10) + "%";
            const links = li.getElementsByTagName('A');
            if (links.length === 1) {
                const a = links[0];
                a.title = "r = " + formatPercent(rate) + " %";
                while (a.firstChild !== null) {
                    a.removeChild(a.firstChild);
                }
                const img = document.createElement('IMG');
                img.src = li.getAttribute('data-preview');
                img.width = img.height = 200;
                const div = document.createElement('DIV');
                div.appendChild(img);
                div.classList.add('preview');
                div.classList.add('pretty');
                li.appendChild(div);
            } else {
                console.log("WARNING: 'LI' tag does not contain a single 'A' tag as it ought to");
            }
        }
    }
}
