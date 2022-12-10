// Regression for BUG#26818744 : MYSQL SHELL DOESN'T ADD THE SERVER_ID ANYMORE

//@ Configure instance on port 1.
|MySQL server needs to be restarted for configuration changes to take effect.|

//@ Configure instance on port 2.
|MySQL server needs to be restarted for configuration changes to take effect.|

//@ Configure instance on port 3.
|MySQL server needs to be restarted for configuration changes to take effect.|

//@ Restart instance on port 1 to apply new server id settings.
||

//@ Restart instance on port 2 to apply new server id settings.
||

//@ Restart instance on port 3 to apply new server id settings.
||

//@ Connect to instance on port 1.
||

//@ Create cluster with success.
||

//@ Add instance on port 2 to cluster with success.
||

//@ Add instance on port 3 to cluster with success.
||

//@ Remove instance on port 2 from cluster with success.
||

//@ Remove instance on port 3 from cluster with success.
||

//@ Dissolve cluster with success
||
