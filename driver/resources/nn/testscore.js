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

const NN = {

    uncoverLayoutTestCase : function(dummy) {
        const info = JSON.parse(dummy.getAttribute('data-info'));
        const number = Number.parseInt(dummy.getAttribute('data-test-case'));
        dummy.classList.remove('wannabe-layout-comparison');
        dummy.classList.add('layout-comparison');
        const addLayout = function(parent, id, name, informal){
            const div = document.createElement('DIV');
            div.classList.add('pretty');
            div.classList.add('layout');
            div.classList.add(name);
            parent.appendChild(div);
            const a = document.createElement('A');
            a.href = '/layouts/' + id + '/';
            a.title = informal;
            div.appendChild(a);
            const img = document.createElement('IMG');
            img.src = '/layouts/' + id + '/picture.png';
            a.appendChild(img);
        };
        (function(){
            const div = document.createElement('DIV');
            div.classList.add('layout-pair');
            dummy.appendChild(div);
            addLayout(div, info.lhs, 'lhs', info.lhsInformal);
            addLayout(div, info.rhs, 'rhs', info.rhsInformal);
            dummy.appendChild(div);
        })();
        (function(){
            const div = document.createElement('DIV');
            div.classList.add('ruler');
            for (const name in info.values) {
                div.setAttribute('data-value-' + name, info.values[name]);
            }
            initComparisonRuler(div, number);
            dummy.appendChild(div);
        })();
        dummy.parentNode.style = "";
    },

    enableDynamicScrolling : function() {
        const notice = document.getElementById('scroll-for-more');
        const addStuff = function(){
            notice.style = "display:none";
            for (const dummy of document.getElementsByClassName('wannabe-layout-comparison')) {
                NN.uncoverLayoutTestCase(dummy);
                notice.style = "";
                return;
            }
        };
        const filler = document.getElementById('vertical-fill');
        const listener = function(event) {
            if (NN.isBottomVisible(filler)) {
                addStuff();
            }
        };
        document.addEventListener('scroll', listener);
        while (NN.isBottomVisible(filler)) {
            addStuff();
        }
        notice.style = "";
    },

    disableDynamicScrolling : function() {
        const notice = document.getElementById('scroll-for-more');
        const filler = document.getElementById('vertical-fill');
        notice.parentNode.removeChild(notice);
        filler.parentNode.removeChild(filler);
    },

    removeErrorMessage : function() {
        const node = document.getElementById('something-went-wrong');
        node.parentNode.removeChild(node);
    },

    isBottomVisible : function(element) {
        const rect = element.getBoundingClientRect();
        return rect.bottom <= (window.innerHeight || document.documentElement.clientHeight);
    },

    init : function() {
        if ((location.hash !== null) && (location.hash.startsWith('#test-case-'))) {
            NN.disableDynamicScrolling();
            const header = document.getElementById(location.hash.substring(1));
            const caseno = Number.parseInt(location.hash.substring('#test-case-'.length));
            if ((header !== null) && Number.isFinite(caseno)) {
                console.log("Uncovering test case #" + caseno + " ...");
                for (const dummy of document.getElementsByClassName('wannabe-layout-comparison')) {
                    const number = Number.parseInt(dummy.getAttribute('data-test-case'));
                    if (caseno === number) {
                        NN.uncoverLayoutTestCase(dummy);
                        NN.removeErrorMessage();
                        break;
                    }
                }
            }
        } else {
            for (const dummy of document.getElementsByClassName('wannabe-layout-comparison')) {
                NN.uncoverLayoutTestCase(dummy);
                break;
            }
            NN.enableDynamicScrolling();
            NN.removeErrorMessage();
        }
    }

};  // "namespace" NN
