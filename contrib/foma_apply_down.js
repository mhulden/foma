// Basic recursive apply down function for Javascript runtime. 
// Caveat: does not support flag diacritics and will recurse infinitely
// on input-side epsilon-loops.
// Use the foma2js.perl script to convert foma binaries to a Javascript array which
// is needed as the first argument of foma_apply_down.

function foma_apply_down(Net, inString) {
    Rep = new Object;
    Rep.answer = new Array;
    Rep.results = 0;
    foma_apply_dn(Net, inString, 0, 0, '', Rep);
    return(Rep.answer);
}

function foma_apply_dn(Net, inString, Position, State, outString, Reply) {
    if (Net.f[State] === 1 && Position === inString.length) {
        Reply.answer.push(outString);
        Reply.results++;
    }
    var match = 0;
    for (var len = 0; len <= Net.maxlen && len <= inString.length - Position; len++) {
        var key = State + '|' + inString.substr(Position,len);
        for (var key2 in Net.t[key]) {
            for (var targetState in Net.t[key][key2]) {
                if (targetState == null) {
                    return;
                }
                var outputSymbol = Net.t[key][key2][targetState];
                match = 1;
                if (outputSymbol === '@UN@') { outputSymbol = '?'; }
                foma_apply_dn(Net, inString, Position+len, targetState, outString + outputSymbol, Reply);
            }
        }
    }
    if (match === 0 && Net.s[inString.substr(Position,1)] == null && inString.length > Position) {
        key = State + '|' + '@ID@';
        for (key2 in Net.t[key]) {
            for (targetState in Net.t[key][key2]) {
	        if (targetState == null) {
                    return;
                }
                outputSymbol = Net.t[key][key2][targetState];
                if (outputSymbol === '@UN@') { outputSymbol = '?'; }
	        if (outputSymbol === '@ID@') { outputSymbol = inString.substr(Position,1); }
                foma_apply_dn(Net, inString, Position+1, targetState, outString + outputSymbol, Reply);
            }
        }
    }
}
