module.exports = {
  flowFile: "flows/main.json",
  flowFilePretty: true,
  credentialSecret: process.env.NODE_RED_CREDENTIAL_SECRET,

  editorTheme: {
    projects: {
      enabled: false
    }
  }
};
