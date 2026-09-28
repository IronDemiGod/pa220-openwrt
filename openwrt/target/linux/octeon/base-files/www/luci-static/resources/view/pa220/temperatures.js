'use strict';
'require view';
'require form';
'require fs';
'require uci';
'require poll';
'require dom';

function readTemps() {
	return fs.exec_direct('/usr/libexec/pa220-temps').then(function(out) {
		return (out || '').trim().split('\n').filter(function(l) {
			return l.length;
		}).map(function(l) {
			var f = l.split('\t');
			return {
				sensor: f[0], chan: f[1], label: f[2],
				mc: parseInt(f[3], 10), fault: f[4] === '1'
			};
		});
	}).catch(function() {
		return [];
	});
}

function tempTable(temps, hot) {
	var rows = temps.map(function(t) {
		var c = isNaN(t.mc) ? null : t.mc / 1000;
		var state = t.fault ? _('sensor fault') :
			(c !== null && c >= hot ? _('hot') : _('ok'));

		return E('tr', { 'class': 'tr' }, [
			E('td', { 'class': 'td left' }, t.label),
			E('td', { 'class': 'td left' }, t.sensor + ' ' + t.chan),
			E('td', { 'class': 'td left', 'style': (c !== null && c >= hot) ? 'font-weight:bold;color:#d9534f' : '' },
				c === null ? '-' : c.toFixed(1) + ' °C'),
			E('td', { 'class': 'td left' }, state)
		]);
	});

	if (!rows.length)
		rows.push(E('tr', { 'class': 'tr' }, E('td', { 'class': 'td', 'colspan': 4 }, _('No temperature sensors found.'))));

	return E('table', { 'class': 'table' }, [
		E('tr', { 'class': 'tr table-titles' }, [
			E('th', { 'class': 'th' }, _('Sensor')),
			E('th', { 'class': 'th' }, _('Channel')),
			E('th', { 'class': 'th' }, _('Temperature')),
			E('th', { 'class': 'th' }, _('Status'))
		])
	].concat(rows));
}

return view.extend({
	load: function() {
		return Promise.all([ uci.load('pa220'), readTemps() ]);
	},

	render: function(data) {
		var hot = parseInt(uci.get('pa220', 'temp', 'hot') || '85', 10);
		var refresh = parseInt(uci.get('pa220', 'temp', 'refresh') || '5', 10);
		var tableNode = E('div', {}, tempTable(data[1], hot));
		var m, s, o;

		poll.add(function() {
			return readTemps().then(function(t) {
				dom.content(tableNode, tempTable(t, hot));
			});
		}, refresh);

		m = new form.Map('pa220', _('Temperatures'),
			_('Board sensors. The TEMP LED on the front panel turns orange when the hottest channel reaches the "hot" threshold and green again below the "cool" threshold.'));

		s = m.section(form.NamedSection, 'temp', 'temp', _('Settings'));
		s.addremove = false;

		o = s.option(form.ListValue, 'refresh', _('Refresh interval'),
			_('How often this page updates the readings (applies after saving).'));
		o.value('2', _('2 seconds'));
		o.value('5', _('5 seconds'));
		o.value('10', _('10 seconds'));
		o.value('30', _('30 seconds'));
		o.value('60', _('1 minute'));
		o.default = '5';

		o = s.option(form.Value, 'hot', _('TEMP LED orange at (°C)'));
		o.datatype = 'range(40,110)';
		o.default = '85';
		o.rmempty = false;

		o = s.option(form.Value, 'cool', _('TEMP LED green again below (°C)'));
		o.datatype = 'range(30,109)';
		o.default = '80';
		o.rmempty = false;
		o.validate = function(section_id, value) {
			var h = parseInt(this.section.formvalue(section_id, 'hot'), 10);
			var c = parseInt(value, 10);
			if (!isNaN(h) && !isNaN(c) && c >= h)
				return _('Must be lower than the orange threshold');
			return true;
		};

		return m.render().then(function(formNode) {
			return E([], [
				E('h2', {}, _('Temperatures')),
				E('h3', {}, _('Current readings')),
				tableNode,
				formNode
			]);
		});
	}
});
