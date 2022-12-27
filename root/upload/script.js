const button = document.getElementById('post-btn');
const upload = document.getElementById('upload');

button.addEventListener('click', async _ => {
    console.log(upload.files[0]);
  try {     
    const response = await fetch('http://localhost:8090/cgi/upload.py', {
      method: 'post',
      body: upload.files[0],
    });
    console.log(response);
  } catch(err) {
    console.error(`Error: ${err}`);
  }
});