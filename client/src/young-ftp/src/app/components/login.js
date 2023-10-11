'use client'
import * as React from 'react';
import Avatar from '@mui/material/Avatar';
import Button from '@mui/material/Button';
import CssBaseline from '@mui/material/CssBaseline';
import TextField from '@mui/material/TextField';
import FormControlLabel from '@mui/material/FormControlLabel';
import Checkbox from '@mui/material/Checkbox';
import Link from '@mui/material/Link';
import Paper from '@mui/material/Paper';
import Box from '@mui/material/Box';
import Grid from '@mui/material/Grid';
import LockOutlinedIcon from '@mui/icons-material/LockOutlined';
import SelectInput from '@mui/material/Select/SelectInput';
import Typography from '@mui/material/Typography';
import { createTheme, ThemeProvider } from '@mui/material/styles';
import CopyRight from './copyright';

const defaultTheme = createTheme();

export default function SignInSide() {
  const handleSubmit = (event) => {
    event.preventDefault();
    const data = new FormData(event.currentTarget);
    console.log({
      host: data.get('host'),
      port: data.get('port'),
      user: data.get('user'),
      password: data.get('password'),
      encoding: data.get('encoding'),
      timeout: data.get('timeout'),
    });
  };

  return (
    <ThemeProvider theme={defaultTheme}>
      <Grid container component="main" sx={{ height: '100vh' }}>
        <CssBaseline />
        <Grid
          item
          xs={false}
          sm={4}
          md={7}
          sx={{
            backgroundImage: 'url(/youth_ftp_large.svg)',
            backgroundRepeat: 'no-repeat',
            backgroundColor: (t) =>
              t.palette.mode === 'light' ? t.palette.grey[50] : t.palette.grey[900],
            backgroundSize: 'cover',
            backgroundPosition: 'center',
          }}
        />
        <Grid item xs={12} sm={8} md={5} component={Paper} elevation={6} square>
          <Box
            sx={{
              my: 8,
              mx: 4,
              display: 'flex',
              flexDirection: 'column',
              alignItems: 'center',
            }}
          >
            <Avatar sx={{ m: 1, bgcolor: 'secondary.main' }}>
              <LockOutlinedIcon />
            </Avatar>
            <Typography component="h1" variant="h5">
              Sign in
            </Typography>
            <Box component="form" noValidate onSubmit={handleSubmit} sx={{ mt: 1 }}>
              <TextField
                margin="normal"
                required
                fullWidth
                id="host"
                label="Host"
                name="host"
                autoComplete="off"
              />
              <TextField
                margin="normal"
                required
                fullWidth
                id="port"
                label="Port"
                name="port"
                autoComplete="off"
              />
              <TextField
                margin="normal"
                required
                fullWidth
                id="user"
                label="User"
                name="user"
                autoComplete="off"
              />
              <TextField
                margin="normal"
                required
                fullWidth
                id="password"
                label="Password"
                name="password"
                type="password"
                autoComplete="off"
              />
              <TextField
                margin="normal"
                fullWidth
                id="encoding"
                label="Encoding"
                name="encoding"
                autoComplete="off"
              />
              <TextField
                margin="normal"
                fullWidth
                id="timeout"
                label="Timeout"
                name="timeout"
                autoComplete="off"
              />
              <Button
                type="submit"
                fullWidth
                variant="Outlined"
                sx={{
                  mt: 3,
                  mb: 2,
                  '&:hover': {
                    borderColor: 'red', // Set the border color when hovering
                    color: 'white', // Set the text color when hovering
                    backgroundColor: '#64b5f6' // Set the background color when hovering
                  },
                  '&:not(:hover)': {
                    borderColor: 'blue', // Set the border color when not hovering
                    color: 'blue' // Set the text color when not hovering
                  }
                }}
              >
                Sign In
              </Button>
              <CopyRight sx={{ mt: 5 }} />
            </Box>
          </Box>
        </Grid>
      </Grid>
    </ThemeProvider>
  );
}