#include "gg.h"

GaduOptions::GaduOptions(PROTO_INTERFACE *proto) :
	autoRecconect(proto, "AReconnect", 0),
	keepConnectionAlive(proto, "KeepAlive", 1),
	showConnectionErrors(proto, "ShowCErrors", 0),
	useDirectConnections(proto, "DirectConns", 1),
	useForwarding(proto, "Forwarding", 0),
	useManualHosts(proto, "ManualHost", 1),
	useMsgDeliveryAcknowledge(proto, "MessageAck", 1),
	useSslConnection(proto, "SSLConnection", 1),
	directConnectionPort(proto, "DirectPort", 1550),
	forwardPort(proto, "ForwardPort", 1550),
	forwardHost(proto, "ForwardHost", L""),
	serverHosts(proto, "ServerHosts", GG_KEYDEF_SERVERHOSTS)
{
}


GaduOptionsDlgAdvanced::GaduOptionsDlgAdvanced(GaduProto * proto) :
	GaduDlgBase(proto, IDD_OPT_GG_ADVANCED, false),
	chkAutoReconnect(this, IDC_ARECONNECT),
	chkKeepConnectionAlive(this, IDC_KEEPALIVE),
	chkMsgAcknowledge(this, IDC_MSGACK),
	chkShowConnectionErrors(this, IDC_SHOWCERRORS),
	chkSslConnection(this, IDC_SSLCONN),
	chkManualHosts(this, IDC_MANUALHOST),
	edtServerHosts(this, IDC_HOST),
	txtServerHostsLabel(this, IDC_HOST_LIST_L),
	chkDirectConnections(this, IDC_DIRECTCONNS),
	edtDirectPort(this, IDC_DIRECTPORT),
	txtDirectPortLabel(this, IDC_DIRECTPORT_L),
	chkForwarding(this, IDC_FORWARDING),
	edtForwardHost(this, IDC_FORWARDHOST),
	txtForwardHostLabel(this, IDC_FORWARDHOST_L),
	edtForwardPort(this, IDC_FORWARDPORT),
	txtForwardPortLabel(this, IDC_FORWARDPORT_L),
	txtReconnectRequired(this, IDC_RELOADREQD)
{
	CreateLink(chkAutoReconnect, proto->m_gaduOptions.autoRecconect);
	CreateLink(chkKeepConnectionAlive, proto->m_gaduOptions.keepConnectionAlive);
	CreateLink(chkMsgAcknowledge, proto->m_gaduOptions.useMsgDeliveryAcknowledge);
	CreateLink(chkShowConnectionErrors, proto->m_gaduOptions.showConnectionErrors);
	CreateLink(chkSslConnection, proto->m_gaduOptions.useSslConnection);

	CreateLink(chkManualHosts, proto->m_gaduOptions.useManualHosts);
	CreateLink(edtServerHosts, proto->m_gaduOptions.serverHosts);

	CreateLink(chkDirectConnections, proto->m_gaduOptions.useDirectConnections);
	CreateLink(edtDirectPort, proto->m_gaduOptions.directConnectionPort);

	CreateLink(chkForwarding, proto->m_gaduOptions.useForwarding);
	CreateLink(edtForwardHost, proto->m_gaduOptions.forwardHost);
	CreateLink(edtForwardPort, proto->m_gaduOptions.forwardPort);

	chkManualHosts.OnChange = Callback(this, &GaduOptionsDlgAdvanced::onCheck_ManualHosts);
	chkDirectConnections.OnChange = Callback(this, &GaduOptionsDlgAdvanced::onCheck_DirectConnections);
	edtDirectPort.OnChange = Callback(this, &GaduOptionsDlgAdvanced::showRecconectRequired);
	chkForwarding.OnChange = Callback(this, &GaduOptionsDlgAdvanced::onCheck_Forwarding);
	edtForwardHost.OnChange = Callback(this, &GaduOptionsDlgAdvanced::showRecconectRequired);
	edtForwardPort.OnChange = Callback(this, &GaduOptionsDlgAdvanced::showRecconectRequired);
}

void GaduOptionsDlgAdvanced::OnInitDialog()
{
	chkKeepConnectionAlive.Disable();
	chkSslConnection.Disable();

	chkManualHosts.Disable();
	bool useManualHosts = chkManualHosts.GetState() && chkManualHosts.Enabled();
	edtServerHosts.Enable(useManualHosts);
	txtServerHostsLabel.Enable(useManualHosts);

	bool useDirectConnection = chkDirectConnections.GetState();
	edtDirectPort.Enable(useDirectConnection);
	txtDirectPortLabel.Enable(useDirectConnection);
	chkForwarding.Enable(useDirectConnection);

	bool useForwarding = useDirectConnection && chkForwarding.GetState();
	edtForwardHost.Enable(useForwarding);
	txtForwardHostLabel.Enable(useForwarding);
	edtForwardPort.Enable(useForwarding);
	txtForwardPortLabel.Enable(useForwarding);

	txtReconnectRequired.Hide();
}

void GaduOptionsDlgAdvanced::onCheck_ManualHosts(CCtrlCheck *)
{
	bool useManualHosts = chkManualHosts.GetState();
	edtServerHosts.Enable(useManualHosts);
	txtServerHostsLabel.Enable(useManualHosts);

	showRecconectRequired();
}

void GaduOptionsDlgAdvanced::onCheck_DirectConnections(CCtrlCheck *)
{
	bool useDirectConnection = chkDirectConnections.GetState();
	edtDirectPort.Enable(useDirectConnection);
	txtDirectPortLabel.Enable(useDirectConnection);
	chkForwarding.Enable(useDirectConnection);

	bool useForwarding = useDirectConnection && chkForwarding.GetState();
	edtForwardHost.Enable(useForwarding);
	txtForwardHostLabel.Enable(useForwarding);
	edtForwardPort.Enable(useForwarding);
	txtForwardPortLabel.Enable(useForwarding);

	showRecconectRequired();
}

void GaduOptionsDlgAdvanced::onCheck_Forwarding(CCtrlCheck *)
{
	bool useForwarding = chkForwarding.GetState();
	edtForwardHost.Enable(useForwarding);
	txtForwardHostLabel.Enable(useForwarding);
	edtForwardPort.Enable(useForwarding);
	txtForwardPortLabel.Enable(useForwarding);

	showRecconectRequired();
}

void GaduOptionsDlgAdvanced::showRecconectRequired(CCtrlBase*)
{
	txtReconnectRequired.Show();
}
