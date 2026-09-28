'use strict';
'require view';
'require form';

return view.extend({
	render: function() {
		var m, s, o;

		m = new form.Map('pa220', _('PA-220 LEDs'),
			_('Switch the front-port jack LEDs and the front-panel status LEDs (HA, ALM, STAT, TEMP) on or off, and optionally switch all of them off during a nightly period. PWR is hardwired. ALM red still lights on a kernel panic.'));

		s = m.section(form.NamedSection, 'leds', 'leds');
		s.addremove = false;

		o = s.option(form.Flag, 'jacks', _('Jack LEDs (ports 1-8)'),
			_('The modes below apply to all eight jacks.'));
		o.default = '1';
		o.rmempty = false;

		o = s.option(form.ListValue, 'left', _('Left jack LED'));
		o.depends('jacks', '1');
		o.default = '0';
		o.value('0', _('On: link'));
		o.value('1', _('On: link, blink: activity'));
		o.value('2', _('Blinks 3/2/1 times: 1000/100/10 Mbps'));
		o.value('3', _('On: activity'));
		o.value('4', _('Blink: activity'));
		o.value('5', _('On: transmit'));
		o.value('6', _('On: copper link'));
		o.value('7', _('On: 1000 Mbps link'));
		o.value('8', _('Always off'));
		o.value('9', _('Always on'));
		o.value('b', _('Always blinking'));

		o = s.option(form.ListValue, 'right', _('Right jack LED'));
		o.depends('jacks', '1');
		o.default = '6';
		o.value('0', _('On: copper link'));
		o.value('1', _('On: link, blink: activity'));
		o.value('2', _('On: link, blink: receive'));
		o.value('3', _('On: activity'));
		o.value('4', _('Blink: activity'));
		o.value('5', _('On: 100 Mbps link'));
		o.value('6', _('On: 100 or 1000 Mbps link'));
		o.value('7', _('On: 100 Mbps link (copper)'));
		o.value('8', _('Always off'));
		o.value('9', _('Always on'));
		o.value('b', _('Always blinking'));

		o = s.option(form.Flag, 'front', _('Front-panel LEDs'),
			_('HA, ALM, STAT and TEMP.'));
		o.default = '1';
		o.rmempty = false;

		o = s.option(form.Flag, 'night_off', _('Night mode'),
			_('All LEDs off between the two times below (router timezone, see System → System).'));
		o.default = '0';
		o.rmempty = false;

		o = s.option(form.Value, 'night_start', _('Off from'), _('HH:MM, 24-hour'));
		o.depends('night_off', '1');
		o.default = '23:00';
		o.validate = function(section_id, value) {
			return /^([01][0-9]|2[0-3]):[0-5][0-9]$/.test(value) ? true : _('Use HH:MM, e.g. 23:00');
		};

		o = s.option(form.Value, 'night_end', _('On again at'), _('HH:MM, 24-hour (may be the next morning)'));
		o.depends('night_off', '1');
		o.default = '07:00';
		o.validate = function(section_id, value) {
			return /^([01][0-9]|2[0-3]):[0-5][0-9]$/.test(value) ? true : _('Use HH:MM, e.g. 07:00');
		};

		o = s.option(form.Flag, 'night_ha', _('HA LED on at night'),
			_('Keep the HA LED solid green during the night period, as a night mode indicator.'));
		o.depends('night_off', '1');
		o.default = '1';
		o.rmempty = false;

		return m.render();
	}
});
