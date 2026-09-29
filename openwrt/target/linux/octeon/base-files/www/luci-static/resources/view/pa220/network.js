'use strict';
'require view';
'require form';
'require fs';
'require uci';
'require poll';
'require dom';

var lastCpu = null;

function readStats() {
	return fs.exec_direct('/usr/sbin/pa220-net', [ 'stats' ]).then(function(out) {
		var st = { ports: [] };

		(out || '').trim().split('\n').forEach(function(l) {
			var f = l.trim().split(/\s+/);

			if (f[0] === 'ports')
				st.ports = f.slice(1);
			else if (f[0] === 'cpu')
				st.cpu = [ parseInt(f[1], 10), parseInt(f[2], 10) ];
			else if (f[0] === 'fastpath')
				st.enabled = (f[1] === 'Y');
			else if (f.length === 2)
				st[f[0]] = parseInt(f[1], 10);
		});

		/* softirq share of all CPU time since the previous reading */
		if (st.cpu && lastCpu && st.cpu[1] > lastCpu[1])
			st.sirq = 100 * (st.cpu[0] - lastCpu[0]) / (st.cpu[1] - lastCpu[1]);
		if (st.cpu)
			lastCpu = st.cpu;

		return st;
	}).catch(function() {
		return null;
	});
}

function num(v) {
	return (v == null || isNaN(v)) ? '-' : String(v);
}

function statsTable(st) {
	if (!st)
		return E('p', {}, _('The driver status could not be read.'));

	var rows = [
		[ _('Fast path'), st.enabled ? _('on') : _('off') ],
		[ _('Active on ports'), st.ports.length ? st.ports.join(', ') :
			(st.enabled ? _('none (no port qualifies, see the conditions above)') : '-') ],
		[ _('Frames forwarded by the fast path'), num(st.forwarded) ],
		[ _('Frames passed to the bridge (learning / refresh)'), num(st.to_bridge) ],
		[ _('Unknown destination (handled by Linux)'), num(st.miss) ],
		[ _('Hardware queue full (handled by Linux)'), num(st.busy) ],
		[ _('Skipped while a packet capture was open'), num(st.tap) ],
		[ _('CPU load from packet handling (softirq, all cores)'),
			st.sirq == null ? _('measuring…') : st.sirq.toFixed(1) + ' %' ]
	];

	return E('table', { 'class': 'table' }, rows.map(function(r) {
		return E('tr', { 'class': 'tr' }, [
			E('td', { 'class': 'td left', 'style': 'width:50%' }, r[0]),
			E('td', { 'class': 'td left' }, r[1])
		]);
	}));
}

return view.extend({
	load: function() {
		return Promise.all([ uci.load('pa220'), uci.load('network'), readStats() ]);
	},

	render: function(data) {
		var statsNode = E('div', {}, statsTable(data[2]));
		var m, m2, s, o;

		poll.add(function() {
			return readStats().then(function(st) {
				dom.content(statsNode, statsTable(st));
			});
		}, 3);

		m = new form.Map('pa220', _('PA-220 Performance'),
			_('The PA-220 has no switch chip: every frame between the front ports is handled by the CPU. These settings tune the ethernet driver. Changes apply immediately after saving, without a reboot.'));

		s = m.section(form.NamedSection, 'net', 'net', _('Bridge fast path'));
		s.addremove = false;

		o = s.option(form.Flag, 'fastpath', _('Enable bridge fast path'),
			_('Frames to a known device on another port of the same bridge are sent straight back out by the driver, bypassing the Linux network stack. Broadcasts, multicast, unknown destinations, traffic for this device and routed/internet traffic still go through Linux.') + '<br />' +
			_('Works with VLAN filtering bridges (802.1Q tags are added or removed per port as configured). Stays off automatically for a port when its bridge uses 802.1ad or MST, the port is not in STP forwarding state, isolated or locked, bridge firewall rules (nftables bridge family / ebtables) exist, or a packet capture (tcpdump) is running. Per-port traffic shaping and bridge statistics do not see fast path frames.'));
		o.default = '1';
		o.rmempty = false;

		s = m.section(form.NamedSection, 'net', 'net', _('Receive tuning'));
		s.addremove = false;

		o = s.option(form.Flag, 'rx_gro', _('GRO (generic receive offload)'),
			_('Merges received TCP segments. Helps traffic that ends on this device; bridged or routed packets have to be split again before sending, so switching off may lower the CPU load for pure forwarding.'));
		o.default = '1';
		o.rmempty = false;

		o = s.option(form.Value, 'rx_irq_delay', _('Receive interrupt delay'),
			_('1-15 hardware timer ticks (default 1). A longer delay collects more packets per interrupt, lowering CPU load at the cost of a few microseconds of latency.'));
		o.datatype = 'range(1,15)';
		o.default = '1';
		o.rmempty = false;

		o = s.option(form.Value, 'rx_irq_pkts', _('Interrupt at once after N packets'),
			_('0-255, 0 = off (default). With a longer delay, still interrupt immediately once this many packets are waiting.'));
		o.datatype = 'range(0,255)';
		o.default = '0';
		o.rmempty = false;

		m2 = new form.Map('network');
		s = m2.section(form.NamedSection, 'globals', 'globals', _('Packet steering'));
		s.addremove = false;

		o = s.option(form.ListValue, 'packet_steering', _('Packet steering'),
			_('Software distribution of received packets over the CPUs (the same setting as in Network → Interfaces → Global network options). The driver already spreads packets over the four cores in hardware, so Disabled is recommended on the PA-220.'));
		o.value('0', _('Disabled'));
		o.value('1', _('Enabled'));
		o.value('2', _('Enabled (all CPUs)'));
		o.default = '1';

		return Promise.all([ m.render(), m2.render() ]).then(function(nodes) {
			return E([], [
				nodes[0],
				nodes[1],
				E('h3', {}, _('Status')),
				statsNode
			]);
		});
	}
});
