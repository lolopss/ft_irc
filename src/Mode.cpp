#include "../headers/Channel.hpp"

// 662 ['+' | '-']<mode_char> :<warning>

void	Channel::handleModeI(bool activate)
{
	if (activate)
		_modeI = true;
	else
		_modeI = false;
}

void	Channel::handleModeT(bool activate)
{
	if (activate)
		_modeT = true;
	else
		_modeT = false;
}

void	Channel::handleModeK(bool activate, const std::string &mdp)
{
	if (activate)
	{
		_modeK = true;
		_channelPassword = mdp;
	}
	else
	{
		_modeK = false;
		_channelPassword = "";
	}
}

void	Channel::handleModeO(bool activate, const std::string &nickname, Client *user, Server *server)
{
	if (activate)
		grantOperator(user, nickname, server, true);
	else
		grantOperator(user, nickname, server, false);
}

void	Channel::handleModeL(bool activate, const int &userLimit)
{
	if (activate)
	{
		_modeL = true;
		_userLimit = userLimit;
	}
	else
	{
		_modeL = false;
		_userLimit = 0;
	}
}
